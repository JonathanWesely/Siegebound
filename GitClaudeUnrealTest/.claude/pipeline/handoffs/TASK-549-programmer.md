# TASK-549 — THE TESTS, and the Zone-A re-base — programmer handoff

- **task:** TASK-549 [AC-9] · **assignee:** gameplay-programmer · **date:** 2026-08-05
- **status on completion:** `ready-for-qa`
- **QA gate:** ⛔ **TASK-550** (`.claude/pipeline/qa/TASK-550.md` — the batch's ONLY gate, naming 541..549)
- **law read in full first:** CONVENTIONS **`AS-§21.0`–`AS-§21.12` (all 13 clauses)** · `AS-§20.1` · `AS-§20.4` · `AS-§9c` · `AS-§12f` · `AS-§12g` · `AS-§3` · `SC-§13` · `SC-§15` · `SC-§18` · `SC-§22` · `SC-§23` · `SC-§29`
- **read before writing a line:** `handoffs/TASK-541-` · `-542-` · `-543-` · `-544-` · `-545-` · `-546-` · `-547-` · `-548-programmer.md` (all eight), then the shipped source of every one of them.
- ⛔ **NO COMPILE. NO TEST RUN. NO GIT. NO EDITOR.** TASK-551 owns the only compile and the only run.
- 🔒 **`Docs/Data/assistant_eval_holdout2.csv` NOT OPENED.** ⛔ No `Docs/Data/*.csv` touched at all.

📌 **M8 DECLARATION, verbatim as required:** *"adds no replicated property, no new replicated class, no new relevancy tier."*
✅ And the reason is structural for this task specifically: **it adds no runtime code at all.** Every line lands inside `#if WITH_DEV_AUTOMATION_TESTS`, so there is no shipped symbol, no `UPROPERTY`, no class and nothing that could enter a relevancy tier.

---

## 1. FILES TOUCHED — THREE, ALL IN `Tests/`

| file | change | scope |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp` | **EXTENDED** — 15 new tests + a fixture block. ⛔ No second file created. | in `names:` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantZoneATest.cpp` | **RE-BASED** — 5424 → 5658, seven declared components, two ceilings, ASCII per component, state-independence | in `names:` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` | ⚠️ **DECLARED SCOPE DEPARTURE — see §7 DISAGREEMENT 1.** Four region-bearing fixtures added to `RuleNameCharset`, its false comment repaired, a reached-the-new-rules guard added. | ⛔ **NOT in `names:`** |

⛔ **No shipped `.cpp` / `.h` was edited.** ⛔ No `Build.cs`, no `Content/`, no `.umap`, no `Docs/`, no `Plugins/`.
✅ **Balance-checked** (comment- and string-aware brace/paren/bracket scan): all three files balance at 0.

---

## 2. ⭐⭐ THE ZONE-A RE-BASE — HONESTLY, AND THE NUMBERS ARE THE AUTHORS' MEASUREMENTS

### 2.1 The named, dated baseline

> **`ShippedZoneAChars = 5658`, named and dated `2026-08-05, batch AI-COMMANDER ROBUSTNESS`.**
> **`SpikeLaneZoneAChars = 5116` — ⛔ UNCHANGED, and the test now says why in its own failure message: `Plugins/SiegeLlama/**` was deliberately not touched by *either* batch (TASK-481's in-flight instrument, `FT-§16`; `AS-§21.10` lists `SiegeLlamaSpike.cpp` under ⛔ NOT TOUCHED by name).**

⛔ **THE FROZEN SPIKE FIXTURE IS BYTE-UNTOUCHED. NOT ONE CHARACTER OF `BuildZoneA`'s OUTPUT WAS COPIED INTO IT.** What grew is the *declared diff*.

⭐ **AND THE FIXTURE'S FREEZE IS NOW ASSERTED, NOT MERELY PROMISED.** A length check alone would pass a fixture edited to stay 5116 while its content drifted toward the shipped builder — the re-copy failure in its cheapest form. So `TwoLaneByteEquality` now also asserts the fixture still carries the **pre-TASK-541 `defend` clause verbatim**, does **not** carry the post-TASK-541 wording, and carries **no `ZONE   = ` line at all**.

### 2.2 The delta, component by component — the `:617-641` idiom, extended to seven

Each is its own named literal, measured by the compiler, so a transcription error fails **with its component and its task number attached**:

| # | task | component | Δ |
|---|---|---|---|
| 1 | TASK-521 | `WHO    =` gains `{"all_except":[KIND]}` | **+44** |
| 2 | TASK-521 | rule line — `who` is `"all"` | **+111** |
| 3 | TASK-521 | rule line — the exclusion, four verbs | **+153** |
| 4 | **TASK-541** | the `defend` note repair | **−5** |
| 5 | **TASK-547** | `WHO    =` gains `, or {"in":ZONE}` | **+16** |
| 6 | **TASK-547** | the whole `ZONE   = ` line, incl. newline | **+76** |
| 7 | **TASK-547** | the in-vs-where rule line, incl. newline | **+147** |
| | | **sum** | **+542** ⇒ 5116 + 542 = **5658** |

⛔ **These are the numbers TASK-541 §3 and TASK-547 §3 MEASURED. I did not derive one of them from a total** — I re-derived each literal from the law and the shipped source and let the compiler count it. The three TASK-547 figures reproduce independently (16 / 76 / 147), and the −5 reproduces from the two `[notes]` clauses (66 → 61).

⚠️ **COMPONENT 4 IS THE ONE A CARELESS RE-BASE MISSES.** TASK-541's edit lives in `SiegeAssistantVocabulary.cpp`, not in `SiegeAssistantSnapshot.cpp`. Zone A prints it through `BuildSynonymTable()`, so **it is Zone A's byte even though it is another file's line.** A re-base that looked only at the snapshot would have been wrong by exactly 5. Written into the file's header so the next one is not.

### 2.3 ⛔ TWO CEILINGS, TWO BASES — asserted separately (`SC-§23`)

| law | ceiling | base | spend | asserted |
|---|---|---|---|---|
| `AS-§20.4` | 325 | the **spike's 5116** | **308** (components 1–3) | ✅ kept, still true |
| `AS-§21.7` | **250** | the **RE-BASED 5419** | **239** (components 5–7) | ✅ **11 chars remain** |

⛔ **Comparing the whole 542 to either ceiling, or either spend to the other's ceiling, is the base error `SC-§23` names.** The re-based 5419 ships as its own named constant `RebasedZoneAChars_2026_08_05`, because a ceiling checked against the wrong base is a green test that measured nothing.

### 2.4 ⭐ WHICH TESTS READ THE CONSTANT — asked the right way round

The dispatch warned that last batch a **third** test broke that nobody listed. It was right, and the general lesson is now written into the file: **a spec lists the tests it KNOWS about; only a grep lists the tests that READ THE NUMBER.**

`grep -rn "ShippedZoneAChars|SpikeLaneZoneAChars|DeclaredD4Delta|D4_|D5_" Source/ Plugins/` ⇒ **`SiegeAssistantZoneATest.cpp` ONLY**, and inside it exactly **three** readers:

1. **`ZoneA.TwoLaneByteEquality`** — re-purposed (§2.1). Named + dated baseline, D4 divergence stated in its own message.
2. **`ZoneA.MeasuredCharCount`** — re-based to 5658, seven components, two ceilings.
3. **`ZoneA.NullVocabularyIsNotTheMeasuredLane`** — ⭐ **the third one, checked and NOT skipped.** It re-based *itself*: its expectation is `ShippedZoneAChars − SynonymTable.Len() + 5`, and `SynonymTable` is read LIVE — so TASK-541's −5 lands on **both sides at once**. Moving one constant fixed it. ⛔ **The lesson is NOT "it was fine": a derived expectation is only safe when EVERY term is derived from something live, and the one term that is not is exactly the one that had to move.** Recorded in place.

**Neither `AsciiCleanliness` nor `StaticPrefixContract` reads a constant.** Both are **re-asserted and strengthened** rather than re-based (§3.2, §3.3).

⛔ **NO FOURTH CASUALTY EXISTS.** Separately swept every other test file for anything derived from the batch: `SiegeAssistantGrammarTest.cpp`'s `Vocabulary.SynonymTable` asserts byte-stability, ordering-independence and the caster-collision — **no length and no `[notes]` content**, so TASK-541 does not touch it. Every other `Build` call site in `Tests/` passes two arguments and is byte-identical by construction (asserted, §3.1 T22c).

---

## 3. THE NEW TESTS, BY NAME — WHAT EACH WOULD CATCH

### 3.1 `Tests/SiegeAssistantSelectionTest.cpp` — 15 new, all `Siegebound.Assistant.Selection.<Name>`, all `EditorContext | EngineFilter`, all inside `#if WITH_DEV_AUTOMATION_TESTS`

| # | test | what it would catch |
|---|---|---|
| 14 | ⭐⭐ **`RegionAdditivityAtTheWire`** | **Written FIRST, as ordered.** Any accepted JSON changing meaning. 8 shipped `who` shapes across 6 intents + the deferred `when` trigger, each compared as a **whole-struct digest** (`CommandDigest`) — so the seventh field acquiring a value on an input that never said `in` fails here, which no `TestTrue(bParsed)` would see. 11 pre-existing refusal codes re-asserted. The 4-key top-level set re-asserted **in both directions**. ⚠️ TASK-545's **sole declared exception** pinned: `{"in":…,"all_except":…}` `unknown_key:in` → `region_conflict:all_except/in`, **refused on both sides of the change**. |
| 15 | ⭐ **`RegionParses`** | The accept side of Jonathan's sentence B on **all four** selection-bearing intents × **all three** regions; `Kinds`/`Counts`/`ExcludeKinds` stay empty (the executor's `Kinds.Num()==0` seam); mixed case normalises; ⭐ **an unknown symbol and an empty region are LEGAL PARSES** (the executor's questions); ⭐⭐ **both place-valued keys populated in one command**. |
| 16 | ⭐⭐ **`RegionRefusedOnArmyWideIntents`** | The silent drop. `region_conflict:<intent>` on charge/fallback/rally, payload = the verb, command fully reset, plus the `SiegeAssistantIntentTakesSelection` gate pin and a ✅ `send` control. Would catch *"fall back, but only the ones in the mid"* executing as **"fall back, EVERYONE."** |
| 17 | **`RegionSymbolsRefused`** | `bad_type:in` on number/array/object/null/bool; `bad_region` on `""`, whitespace, `none`, `all`, **`now`**; the exact-key-set rule inside the region object; and ⛔ the `{"in":"none"}` ≠ `who:"all"` assertion — the case that would turn a typed filter into an unfiltered army in silence. |
| 18 | **`RegionConflictsRefused`** | region+selection (`1/mid`), region+`all_except` (`all_except/mid`), the parser's mixed-object code, region+`who:"none"`, the wire-side reserved symbols, and ⚠️ the honest limit that a wire `"none"` **is** `NAME_None` and cannot be distinguished. |
| 19 | ⭐⭐ **`ValidatorRegionArgumentIsLoadBearing`** | **The silent-no-op class, both trailing defaults.** All **three** call forms (3/4/5-arg) on four region-invalid commands ⇒ dropping the **fifth** is measured; then the 4-kind exclusion through all three ⇒ dropping the **fourth** is re-measured **in the fifth's presence**; then the converse (a clean command validates identically through all three). ⛔ Would fail if either argument's meaning changed. |
| 20 | ⭐⭐ **`RegionMembershipContract`** | **NON-SQUARE extent `(400,100)`, OFF-ORIGIN centre.** Inside/outside per axis; **all four edges and all four corners on the boundary ⇒ INSIDE**; one unit past ⇒ outside; Z at ±1e9 and at `TNumericLimits<double>::Max()/Lowest()` ⇒ still inside; **zero / negative / one-axis-zero extents** (TASK-544's deliberate un-guarded choice, pinned); and ⭐⭐ **two transposed-axis probes that fail in OPPOSITE directions**, plus the same pair against the shipped square `(840,840)` showing both agree — i.e. **the test demonstrates in its own log why shipped geometry cannot catch a transposition.** |
| 21 | ⭐ **`RegionResolvesFromSnapshotGeometry`** | `ResolvePlaceRegion` end-to-end with **no world** (reflection into `PlaceLocations` / `PlaceHalfExtents` / `RegionPlaceNames`): the three regions resolve with their captured non-square boxes and compose with `IsPointInRegion`; ⛔ **all four non-region places FAIL and leave both out-params UNTOUCHED** (asserted against sentinels); unknown / `NAME_None` / `all` fail; ⭐ a region-free snapshot **fails closed on every symbol**; and the published list is a subset of `PlaceNames`. |
| 22 | ⭐ **`RegionGrammarShapes`** | Three / one / zero regions. `zone`'s alternatives are **exactly the passed symbols in CALLER ORDER**; `who` is `selection \| except \| inplace \| "all" \| "none"` **position by position**; ⛔ **zero regions ⇒ `TestEqualSensitive` byte-equality with the 2-arg build**, no `inplace`, no `zone`, `who` = 4; ⭐ TASK-546's **D2** pinned (a reserved-symbols-only list is byte-identical too — never an empty `zone ::= ` RHS); **D3** pinned (`zone` has no `"none"`, asserted on *alternatives*, and `where` **does**, so the asymmetry is between two live rules). |
| 23 | ⭐⭐ **`RegionRuleNamesAreCharsetLegal`** | **Gap 1, closed inside my own file.** Six boards including four region-bearing; every rule NAME against `[a-zA-Z0-9-]`; `who`→`inplace`→`zone` references resolve; `in_place`/`zone_list` rejected by name; ⛔ **a guard that FIVE of the six boards actually emitted `inplace`**, so the test cannot reproduce the very hole it closes. |
| 24 | ⭐⭐ **`RegionGrammarCompositionCarriesRegions`** | **The TASK-548 batch-killer.** Runs the shipped caller's exact three-expression argument list off a real snapshot and asserts `inplace`/`zone`/the `who` alternative are present; then measures the **delta the third argument buys** — the 2-arg call is the bug verbatim and must produce a DIFFERENT grammar; then the converse on a region-free map. ⚠️ **Its residual is stated in the test itself — see §5.** |
| 25 | ⭐⭐ **`ZoneCIsByteUnchangedByRegions`** | **13 kinds, `PerKindTotal` 9 AND 12.** Zone C from a snapshot with regions + geometry is `TestEqualSensitive`-identical to one without — with a **precondition assertion** that the region snapshot really published 3 (otherwise the comparison is vacuous). Plus the **coordinate airlock as a byte property**: none of `91234` / `75319` / `1337` / `8642` / `4242` appears, and there is no `in_region:` / `occupants:` / `regions:` key. Plus the 13-row roster and `sorcerer` still intact. |
| 26 | ⭐ **`ZoneAWhoLineMirrorsGrammar`** | `AS-§9c`. The five shapes appear in Zone A's `WHO =` line at **strictly increasing offsets in the grammar's own alternation order** (a set comparison would pass the wrong sequence); the grammar's `who` asserted position by position; the whole `ZONE   = ` line character-for-character; ⛔ the four non-region places are **not** on it; the in-vs-where rule present verbatim; and TASK-547's **declared omission** (the missing negative half) pinned so it is not "fixed" without a re-count. |
| 27 | ⭐ **`DefendNoteScoped`** | The `[notes]` line no longer contains `ambiguous between guard and fallback`; the repaired clause is present; **exactly 5 chars shorter, measured TWO ways** (the two literals, and the whole table with the old clause restored); ⛔ `"intent":"defend"` is still refused (`AS-§21.3` — not an eighth intent); the three neighbouring `[notes]` rows intact. ⛔ **No accuracy assertion of any kind.** |
| 28 | ⭐ **`RegionRenderedInPlayerSummary`** | ✅ **TASK-548 confirmed headless reachability, so it is asserted rather than deferred.** `Send all in ancient_ground_near (enemy_castle)` / `Guard all in mid (own_castle)` / `Follow all in ancient_ground_far`, all `TestEqualSensitive`; ⛔ four **no-region** renderings byte-identical; both unreachable `, in %s` branches; the raw symbol; and `SC-§31` (an unresolvable region is still described, not softened). Component held in a `TStrongObjectPtr` per TASK-548's note. |

**Fixture added:** `ThreeRegionPlaces()` · `RegionJson()` · `PayloadOf()` · **`CommandDigest()`** · `GrammarRuleRhs/Alternatives()` · `IsLegalGbnfRuleNameHere()` · `FindVectorArrayField/FindVector2DArrayField()` · **`MakeSnapshotWithRegions(..., bPublishRegions)`**.
⚠️ **TEST 12's identical local lambdas were deliberately LEFT IN PLACE** — hoisting them would be a diff in a shipped, passing test for zero behaviour, and this batch's safety argument is that its diff is additive.

### 3.2 `ZoneA.AsciiCleanliness` — re-asserted **per component**
The whole-string check is true and unhelpful when it fails: it reports a character index into a 5658-char prompt. All **seven** declared literals are now checked individually, so a curly apostrophe or en dash pasted across a `TEXT()` boundary **names itself**.

### 3.3 ⭐⭐ `ZoneA.StaticPrefixContract` — a hole that only opened today
The shipped two-snapshot check compares two **empty** snapshots, so it cannot see a Zone A that reads a member which happens to be empty in both. Until TASK-547 there was no such member; there is one now, and its values are exactly the three symbols the new `ZONE   = ` line prints. **So `RegionPlaceNames` is populated by reflection and the bytes are compared** — at three regions **and at one** (the case a live-list generator would shorten). ⚠️ **Declared coupling: if TASK-550 rules TASK-547's departure invalid, THIS assertion is the one that fails.** That is the contract and the ruling disagreeing where a reader can see both — not a bug in the test.

---

## 4. THE THREE GAPS THE IMPLEMENTERS COULD NOT CLOSE — STATUS

1. **`Grammar.RuleNameCharset` blind to `inplace`/`zone`** — ✅ **CLOSED TWICE.** (a) `Selection.RegionRuleNamesAreCharsetLegal` (six boards, in my own file, with a reached-the-new-rules guard); (b) four region fixtures added to `Grammar.RuleNameCharset` itself, its false *"every shape the builder can produce"* comment repaired, and a `3 of 8 emitted inplace` guard added. ⚠️ **(b) is a declared scope departure — §7 DISAGREEMENT 1.**
2. **`IsPointInRegion` with a NON-SQUARE extent** — ✅ **CLOSED.** `RegionMembershipContract`, plus the degenerate extents pinned exactly as TASK-544 chose them, plus the square-extent control that shows *in the log* why shipped data cannot catch a transposition.
3. **The TASK-548 hazard** — ⚠️ **PARTIALLY CLOSED, AND THE RESIDUAL IS STATED — §5.**

---

## 5. ⛔⛔ WHAT I COULD NOT COVER — STATED PLAINLY, NOT PAPERED OVER

1. ⭐⭐ **`ComposeTurnGrammar` ITSELF IS NOT OBSERVABLE, AND THE TASK-548 HAZARD IS THEREFORE NOT MECHANICALLY CAUGHT.**
   `USiegeAssistantComponent::ComposeTurnGrammar` is **`private`** (`SiegeAssistantComponent.h`, below the `private:` at ~`:1075`) and reads the component's `Snapshot` member, which only `CaptureTurnSnapshot()` fills — and that needs a `UWorld` and a resolved team. **I cannot call it.**
   What TEST 24 does instead: runs the caller's **exact three-expression argument list** off a real snapshot, and **measures the delta the third argument buys** (the TEST 11 idiom — the shipped precedent in this same file for an unobservable call site).
   ⛔ **THE RESIDUAL: a future edit that drops the third argument again would compile, link, run, and leave this suite GREEN.** That is TASK-550 criterion (3)'s grep and Jonathan's TASK-552 playtest — **not this file.** It is written into the test's own comment and into an `AddInfo`, so it appears in the run log rather than only here.
   ⚠️ **The same is true of `SiegeAssistantComponent.cpp:1051`'s five-argument call** — TEST 19 measures what the argument buys, it cannot observe the line.
2. ⛔ **THE EXECUTOR'S REGION FILTER IS UNTESTED.** `SelectUnitsForOrder` walks a `UWorld` for `ASummonedUnit` actors; these are `EditorContext` **simple** tests with no world and no actors. The **predicate** (T20) and the **geometry lookup** (T21) are both fully covered — but **the loop that joins them, the fail-closed refusal, the empty-after-region refusal and the arithmetic in the logs are TASK-548's five log lines plus Jonathan at TASK-552.** ⛔ I did not fake a world.
3. ⛔ **NOTHING HERE PARSES THE GRAMMAR WITH llama.cpp.** Law Zero is the cheap mechanical stand-in for the one failure mode review demonstrably misses; **string-comparing two generators can never prove either is valid — only the target parser can.**
4. ⛔ **NO TOKEN FIGURE IS ASSERTED OR DERIVED ANYWHERE** (`AS-§12g`, spec item 9). Only `Siege.Llama.SpikePrompt` prints the real number, and TASK-552 prints it.
5. ⚠️ **THE `NaN`-IS-OUTSIDE CLAIM IS DELIBERATELY NOT ASSERTED.** Constructing a NaN and comparing it is compiler- and float-model-dependent, so the assertion could pass, fail **or be optimised away** for reasons that have nothing to do with the predicate. A test whose outcome depends on the optimiser is worse than a documented note. Recorded in an `AddInfo`, not silently skipped.
6. ⚠️ **A SIXTH TRAILING DEFAULT CANNOT BE DETECTED BY ANY TEST.** `AS-§21.9` forbids one; TEST 19 measures the 3/4/5-argument forms and **cannot see a sixth being added**. That is a review criterion, and it says so in its own `AddInfo` rather than implying coverage it does not have.
7. ⚠️ **THE `DA_AssistantVocabulary` ASSET LANE IS STILL UNTESTED.** `ZoneA.TwoLaneByteEquality` proves the **CODE-DEFAULT** lane; the asset overrides the defaults wholesale at runtime. Pre-existing gap (the file's own header says so), **unchanged by this batch**, restated so nobody reads the new green bars as covering it.
8. ⚠️ **`ZoneCIsByteUnchangedByRegions` compares TWO SNAPSHOTS, not before-and-after the batch.** Nothing could do the latter once the batch landed. The comparison is *stronger* in the way that matters (it fails the day any per-region datum reaches the roster block) and *weaker* in one way: **it cannot see a Zone C change that is independent of region state.** Zone B is not asserted at all here — TASK-547 §5 shows structurally that no hunk lands in either builder, and that is a **review** claim.

---

## 6. ⚠️ THE ONE THING THE SPEC ASKED FOR THAT I READ DIFFERENTLY

Spec item (2) says *"ZONE C IS BYTE-UNCHANGED at 13 kinds, `PerKindTotal` 9 and 12"*. **There is no pre-batch Zone C to compare against** — the batch is on disk. I implemented it as **invariance to region state at both operating points**, which is the claim that actually needs guarding (*"the region feature costs zero prompt budget"*), plus explicit coordinate-absence assertions. **If TASK-550 wanted a frozen Zone-C transcription, that is a different instrument and it would be the "guardrail that reports SAFE" shape `AS-§12g` rules worse than no test.** Flagged, not assumed.

---

## 7. ⭐⭐ EVERY DISAGREEMENT WITH THE EIGHT IMPLEMENTATIONS

> ⚖️ **I am the batch's independent check, so these are the deliverable, not an appendix. ⛔ I fixed NONE of them — TASK-550 rules.**

### DISAGREEMENT 1 — ⚠️ **I EDITED A FILE OUTSIDE MY `names:` BLOCK, AND I AM DECLARING IT RATHER THAN HOPING IT PASSES**
`Tests/SiegeAssistantGrammarTest.cpp` is **not** in TASK-549's `names:` block and **not** in `AS-§21.10`'s *"touched, existing"* list. It **is** inside my dispatched scope (*"`Source/GitClaudeUnrealTest/Siegebound/Tests/` only"*), TASK-546's **D4** assigns the work to me by name (*"TASK-549 must add region-bearing shapes to that array"*), and the dispatch says *"Add region-bearing shapes to that test."*
**Why I took it:** leaving it closes the coverage but leaves a **false comment shipped in a passing test** — *"every shape the builder can produce"* — which is `SC-§22`'s stale-divergence defect in the one test guarding a charset this project has already been bitten by twice. The edit is **purely additive** (one helper, four fixture rows, one guard block, one comment) and no existing assertion changed.
⛔ **If TASK-550 rules it out of scope the revert is mechanical and self-contained**, and `Selection.RegionRuleNamesAreCharsetLegal` keeps the coverage on its own.

### DISAGREEMENT 2 — ⚠️ TASK-545's **`"now"` SUPERSET**: I agree with the code, and I say so in the test's own log
`AS-§21.5` **enumerates** `""` / `"all"` / `"none"` while **justifying** them as *"the three reserved wire symbols (`SiegeAssistantSymbols`) can never name a place"* — and the third reserved symbol is **`now`**, not `""`. **The law contradicts itself.** TASK-545 shipped the superset. **I pinned what ships** and put the contradiction in an `AddInfo` so it surfaces in the run log, not only in a handoff. ⚖️ **My view, on the record: the superset is right** — `Build` drops all three reserved symbols before they can become `zone` alternatives, so `now` can never name a live region, and refusing it costs nothing reachable. ⛔ If TASK-550 rules the enumeration exhaustive, delete one row in TEST 17 and one `||` clause in each of two places.

### DISAGREEMENT 3 — ✅ TASK-547's `ZONE =` DEPARTURE IS **RIGHT**, and I pinned it as a contract rather than as a preference
Board spec (4)(b) says the line is generated from `GetRegionPlaceNames()`; TASK-547 generated it from the `PlaceVocabulary` **table** instead. **I read `BuildZoneA`'s own declaration comment first and it settles it:** Zone A *"READS NO MEMBER STATE, BY CONSTRUCTION"* and making it state-dependent is *"a QA FAIL"* — a varying Zone A destroys the measured KV reuse **silently, as a latency regression**. `GetRegionPlaceNames()` is per-match state. ⇒ **The board's wording, taken literally, asks for the QA FAIL.**
⭐ I asserted the **contract**, not the departure: `StaticPrefixContract` now proves Zone A is byte-identical with 3 regions published, with 1, and with 0. ⚠️ **Declared coupling: a ruling for the board's literal wording fails that assertion.**

### DISAGREEMENT 4 — ⚠️ TASK-547's **degenerate-extent publishing filter** is unspecced, and my tests are agnostic to it
`Capture()` publishes a region only if the captured extent is `> 0` on both axes (TASK-547 §6, flagged by its author). **I did not pin it either way** — `MakeSnapshotWithRegions` writes the arrays directly, bypassing `Capture()`. ⚖️ **My view: keep it.** It is the same fail-closed direction as everything else — a zero-extent region would be **offerable, sampleable, and then contain nobody**, i.e. a refusal the player cannot act on, versus a shape that is simply unsayable. ⛔ But it is a two-line revert if ruled out, and **no test of mine constrains the ruling.**

### DISAGREEMENT 5 — ⚠️ TASK-548's **predicate placement**: the dispatch is right and the board is wrong, and the two cannot both be satisfied
Board spec item (1) says the `Kinds.Num()==0` branch; the dispatch demanded **both** selector branches. **A predicate inside the `all` branch cannot apply to the per-kind branch — the requirements are contradictory, not merely different.** TASK-548 followed the dispatch. ⚖️ **I agree with TASK-548.** ⚠️ But note the consequence honestly: `SC-§21` is *guard PLACEMENT, not guard presence*, and **TASK-550's criterion (4) quotes the board's placement**. ⛔ **The gate must rule on the requirement, not on the code** — and **no test of mine can see the placement at all** (§5 item 2).

### DISAGREEMENT 6 — ✅ TASK-548's **backstop widening** to region+`ExcludeKinds` is right
`AS-§21.5` says in terms *"`all_except` plus a region is the same failure"*. Refusing only the selection leg would leave the exclusion leg **composing quietly** — the exact defect the backstop exists to catch. **TEST 18 and TEST 19 both assert the exclusion leg is refused at the parser AND at the validator**, so the backstop is the third gate on a path two gates already close. ⛔ Unreachable, and correct.

### DISAGREEMENT 7 — 📌 A `SC-§22` OBSERVATION IN A SHIPPED FILE, **REPORTED, NOT FIXED**
`SiegeAssistantSnapshot.cpp`'s `BuildZoneA` header comment (~`:976`) reads *"…are RE-BASED BY TASK-523 against 5424"*. **That sentence is now stale in a file I am forbidden to edit** — TASK-549 re-based them against **5658**. The block immediately below it says so correctly, so the comment contradicts itself two lines apart. ⛔ **One-line comment fix, in TASK-547's file, for TASK-550 to assign.** Zero emitted bytes either way.

### DISAGREEMENT 8 — 📌 `SiegeAssistantGrammar.cpp` (~`:263`) CITES THE CHARSET TEST AS COVERING *"all four builder shapes"*
That claim became **false** the moment `Build` gained a third parameter, and it is the same `AS-§21.1` shape the batch's own diagnosis is about: *an assertion about the rest of the system, falsified by an edit elsewhere, with nobody re-reading it.* My edit to `Grammar.RuleNameCharset` makes the claim true again by construction (eight shapes), **but the sentence in the grammar's own source still says "four".** ⛔ **Reported; not fixed — it is TASK-546's file.**

### DISAGREEMENT 9 — ✅ AGREEMENTS WORTH RECORDING, BECAUSE A CHECKER WHO ONLY DISAGREES IS NOT CHECKING
- **TASK-544's axis order is CORRECT** — verified token by token against both donors (`Point.X` vs `HalfExtent.X`, `Point.Y` vs `HalfExtent.Y`, `<=`, `&&`, no Z term). TEST 20's transposition probes now make that **mechanical** instead of reviewed.
- **TASK-545's additivity argument holds at the code**: `CommandKeys` still holds four entries and `In` is a NESTED key; the object branch's `else` is a fall-through, so `{}` still reports `missing_key:all_except` (asserted).
- **TASK-546's D1** (the fused-terminal `"{\"in\":"` idiom) matches the shipped `except` precedent exactly; **D2** and **D3** are both stronger than the spec and both pinned.
- **TASK-541's −5 reproduces independently** from the two `[notes]` clauses (66 → 61).
- ⛔ **`AS-§12f` SWEEP: I found NO accuracy figure anywhere** — not in the eight handoffs, not in the shipped comments I read, not in this file. **None is introduced here**, and TEST 15 and TEST 27 both carry an `AddInfo` saying so.

---

## 8. WHAT QA SHOULD SCRUTINISE (TASK-550) — hardest first

1. ⛔⛔ **RE-DERIVE THE SEVEN COMPONENTS YOURSELF.** 44 / 111 / 153 / **−5** / 16 / 76 / 147 = **542**; 5116 + 542 = **5658**. ⛔ Do **not** take them from this handoff. A wrong literal must fail with its component named — that is the whole design of the block.
2. ⛔⛔ **CONFIRM THE FROZEN SPIKE FIXTURE IS BYTE-UNTOUCHED** (`git diff` it) and that the three new fixture-freeze assertions in `TwoLaneByteEquality` really run.
3. ⛔ **RULE DISAGREEMENT 1** (the `SiegeAssistantGrammarTest.cpp` scope departure) explicitly — ratify or revert; do not leave it ambiguous.
4. ⛔ **RULE DISAGREEMENT 2** (`"now"`) and **DISAGREEMENT 3** (the `ZONE =` generator's input). ⚠️ **Both have tests riding on them**, and D3's ruling can fail `StaticPrefixContract` by design.
5. ⭐ **CONFIRM TEST 14 ASSERTS WHAT IT SAYS IT ASSERTS** (your criterion 1). Read `CommandDigest`: it must name **every** field of `FSiegeAssistantCommand`. If an eighth field is ever added, that function owes a term and nothing will fail on its own — stated in its comment.
6. ⚠️ **THE TWO CEILINGS AND THEIR BASES** (`SC-§23`). 325-against-5116 and 250-against-5419 are different quantities; check I did not fold them.
7. ⚠️ **§5's RESIDUALS ARE THE HONEST LIMIT, NOT A TO-DO LIST.** In particular: **nothing in `Tests/` can catch a dropped third `Build` argument at the private call site.** Your criterion (3)'s grep is the only mechanism, and it is worth running twice.
8. ⚠️ **DISAGREEMENT 5**: the board and the dispatch give contradictory placement requirements for the region predicate. **Rule the requirement.** No test can see placement.

---

## 9. BOARD + SLACK

- `TASKBOARD.md` → TASK-549 `backlog` → `in-progress` → **`ready-for-qa`** (written twice as specified; I was the only writer of the board in this window, since 550/551 are blocked on me).
- Slack `C0BF0QZP3CN`, ⚙️ Dev & QA `thread_ts = 1783116269.740549`, prefixed `⚙️ GAMEPLAY-PROGRAMMER: ✅ TASK-549`.
