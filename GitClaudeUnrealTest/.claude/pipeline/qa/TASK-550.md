# QA Report — TASK-550

**Verdict: PASS** — 0 BLOCKER · 6 WARN · 7 NIT
**Gate covers (`SC-§29`, the union IS the roster):** **TASK-541 · TASK-542 · TASK-543 · TASK-544 · TASK-545 · TASK-546 · TASK-547 · TASK-548 · TASK-549**
Reviewer: qa-reviewer · 2026-08-05 · law read in full first: CONVENTIONS `AS-§21.0`–`AS-§21.12` (all 13 clauses) + `SC-§29` · `SC-§27` · `SC-§23` · `SC-§21` · `SC-§20` · `SC-§15` · `SC-§14` · `SC-§13` · `AS-§12f` · `AS-§12g` · `AS-§19` · `AS-§23` · `AS-§9c`
⛔ **Nothing compiled, nothing run. A PASS here means CORRECT AS SOURCE.** No code was edited by this gate; no engine, no Git.

---

## 0. ⛔ WHAT I VERIFIED MYSELF vs WHAT I COULD NOT

⛔ **Every claim below was re-derived at the artifact. No handoff figure was accepted on trust, and ⛔ no cited line number was trusted — every symbol was located by grep** (the dispatch's warning was correct: line numbers have drifted repeatedly across this batch, including in the dispatch itself).

⚠️ **WHAT I STRUCTURALLY COULD NOT CHECK, STATED RATHER THAN IMPLIED: I have no Git tool, by design.** Four claims in this batch are *diff-scoped* and only a diff can discharge them: 🔒 `assistant_eval_holdout2.csv` untouched · no `Docs/Data/*.csv` touched · no `.umap` · no `Build.cs`. **They are handed to TASK-551 as named pre-commit checks (§6).** What I *could* do at the content level, I did — see §1 item 9.

---

## 1. THE TEN CRITERIA — RESULT BY RESULT

| # | criterion | result |
|---|---|---|
| **1** | ⭐⭐ **ADDITIVITY AT THE WIRE** | ✅ **VERIFIED AT THE CODE, FOUR WAYS.** `CommandKeys` still holds exactly four entries (`SiegeAssistantCommand.cpp:667-671` — `Intent`/`Who`/`Where`/`When`); `In` is a **nested** key and is never added to it. The new region path is reachable *only* through `bHasRegionKey` (`:799`), and the exclusion arm is the **`else` fall-through**, unmoved (`:837-851`) — so `{}` still reports `missing_key:all_except`. Both new cross-field blocks are guarded by `!RegionPlace.IsNone()` (`:559`, `:1066`), so for any input lacking `{"in":…}` **not one new line executes**. `RegionPlace` defaults to `NAME_None`; no field became required; no default changed. ⭐ **And TASK-549's TEST 14 asserts what it says it asserts** — I read `CommandDigest` (`SelectionTest.cpp:497-522`): it names **every** member (`Intent`, `Kinds`, `Counts`, `ExcludeKinds`, `RegionPlace`, `Where`, `TriggerKind`, `TriggerAtLeast`), so an input that never said `in` acquiring a region fails there. **TASK-545's single declared exception is the only one I can find:** `{"in":…,"all_except":…}` `unknown_key:in` → `region_conflict:all_except/in`, and it is **refused on both sides**. ⛔ **No input that SUCCEEDS today behaves differently.** |
| **2** | ⭐⭐ **FAIL-CLOSED ON AN UNRESOLVED REGION** | ✅ **TRACED MYSELF.** `SiegeAssistantComponent.cpp:2107` — `if (bHasRegion && (!Snapshot \|\| !Snapshot->ResolvePlaceRegion(...))) { UE_LOG(...); return false; }`. There is **no fall-through to the unfiltered set**, no default box, no zero-extent substitute; a missing `Snapshot` takes the same refusal via the short-circuit. The supplier agrees: `ResolvePlaceRegion` (`SiegeAssistantSnapshot.cpp:749-778`) leaves **both out-params untouched on failure** and has no "whole map" fallback. |
| **3** | **SILENT-DROP AUDIT** | ✅ **NO PATH DROPS A REGION.** Army-wide verbs: refused at the parser by cross-field check 3, gated on the **existing** `SiegeAssistantIntentTakesSelection` with **no second intent list** (`SiegeAssistantCommand.cpp:1066-1071`). ⭐ **Both trailing-default hazards re-grepped by me, not taken from the handoffs — see §2 RULING 1.** |
| **4** | **GUARD PLACEMENT (`SC-§21`)** | ✅ **RULED, not passed by silence — see §2 RULING 3.** The predicate is at `SiegeAssistantComponent.cpp:2190`, in the shared candidate gather, immediately **after** the eligibility gate and **before** the candidate is built. One pass; no registry, no cache, no dirty flag, no subscription list. Sorting byte-untouched (`:2264-2267`). |
| **5** | **`AS-§12f` — NO ACCURACY FIGURE** | ✅ **SWEPT INDEPENDENTLY OF TASK-549's SWEEP.** Case-insensitive `accurac` across `Source/GitClaudeUnrealTest/Siegebound/**` returns **21 hits, and every one is either pre-existing prose about accuracy as a concept or an explicit prohibition** (`ZoneATest.cpp:474`, `SelectionTest.cpp:2383`, `:3862`, `:3949`). Same sweep across the nine handoffs: **no predicted score, no row list, no percentage, no expected-improvement claim.** ⛔ The `AS-§12f` deliverable stays the diff. |
| **6** | **THE MEASURED CONSTANTS** | ✅ `FullOffloadCostMiB = 2703` (`SiegeLlamaSubsystem.cpp:76`), `PartialOffloadCostMiB = 1264` (`:77`), `VramGameReserveMiB = 768` (`:104`), `PartialGpuLayers = 18`, `MeasuredModelLayers = 36` — **all byte-unchanged**. The new headroom is a separately-named constant (`:152`) whose comment says **DERIVED, NOT MEASURED**, names its operands, names the 128-head-dim **assumption**, states the `AS-§19` safe direction and states that TASK-552's live `vram_delta` replaces it. ⛔ No recomputation of 2703 anywhere. |
| **7** | **THE ZONE-A RE-BASE IS HONEST** | ✅ The frozen spike fixture is **not** a copy of the builder: the shipped lane is reconstructed as `spike + 7 anchored, individually-named literals`, and **the derivation is checked before it is used** (`ZoneATest.cpp:739-744` — all 7 replacements must have matched, else the test errors out and returns). Baseline is **NAMED and DATED** (`ShippedZoneAChars = 5658`, *"2026-08-05, batch AI-COMMANDER ROBUSTNESS"*) and the D4 divergence is stated **in the test's own failure message** (`:753-758`). ⭐ **And the fixture's freeze is now asserted, not promised** (`:793-798`): it must still carry the pre-TASK-541 `defend` clause, must **not** carry the post-TASK-541 wording, and must carry **no `ZONE   = ` line**. |
| **8** | **BUDGET** | ✅ **RE-COUNTED BY HAND, CHARACTER BY CHARACTER — I did not take 16/76/147 from any handoff.** `, or {"in":ZONE}` = **16**. `ZONE   = an area place symbol: mid, ancient_ground_near, ancient_ground_far\n` = 31 prefix + 3 + 2 + 19 + 2 + 18 + 1 = **76**. The in-vs-where rule line = **147** incl. newline. The `[notes]` clause 66 → 61 = **−5**. ⇒ `5116 + 44 + 111 + 153 − 5 + 16 + 76 + 147 = **5658**`, and the AI-COMMANDER spend is **239 against the re-based 5419 — 11 spare of 250.** ⛔ Two ceilings on two bases, asserted separately (`:905-922`), exactly as `SC-§23` requires. ⛔ **No token figure is derived anywhere** — `zoneA_tok = 1139` stays labelled STALE in all three files that mention it. ✅ **The spike lane stays 5116 (D4)** and I confirmed at the artifact that `SiegeLlamaSpike.cpp` contains **no `inplace`, no `ZONE   = `, no `{"in":`** — with a **positive control** (`SC-§14`): the same file *does* match `USiegeAssistantGrammar::Build` at `:6076` and `ancient_ground_near`, so the negative grep was reaching a file it can match. |
| **9** | **GRAMMAR SAFETY** | ✅ Empty region list ⇒ **byte-identical**: both the rule block (`SiegeAssistantGrammar.cpp:748`) and the `who` alternative (`:823`) sit behind one `bHasRegions`, and `bHasRegions` is the **post-canonicalisation** count (`:443`) so a reserved-symbols-only list also emits nothing — no empty `zone ::= ` RHS is reachable. ✅ Rule names `inplace` / `zone` are `[a-z]` only; the JSON key is `in` (no underscore). ✅ `Build`'s third parameter matches the `AS-§21.9` pin character-for-character (`SiegeAssistantGrammar.h:158-159`). |
| **10** | **STANDING C++ SWEEP** | ✅ `SiegeAssistantRegionStatics.h` includes only `CoreMinimal.h` and names no incomplete type; the `.cpp` includes only its own header; `SiegeAssistantRegionStatics.h` is included at `SiegeAssistantComponent.cpp:17` (the only call site). No most-vexing-parse (the new locals are copy-initialised from named statics). No shadowing (new locals `RegionCentre` / `RegionHalfExtent` / `bHasRegion` / `OutsideRegionCount` / `IneligibleCount` collide with nothing). Format strings: the new `UE_LOG` at `:1017` is a bare `TEXT()` literal with `%d`←`int32`, `%d`←`Output.Len()`, `%s`←`*Output`; `ChooseTier`'s three `OutReason` strings are **5/5, 6/6, 5/5** specifiers-to-args, every operand `uint64` against `%llu`. `AddExpectedMessagePlain` is a **pre-existing shipped idiom in the same file** (5 prior uses), not a new API bet. ⚠️ **`FSiegeAssistantRegionStatics`' header is the pure-data-types exception and is NOT flagged**, per this task's own spec. |
| **11** | **M8 DECLARATION** | ✅ **Present and TRUE in all nine deliverables.** Verified structurally, not by reading the sentence: `ESiegeAssistantIntent : uint8` (`SiegeAssistantCommand.h:66`), and the struct's members are `uint8`/`TArray<FName>`/`TArray<int32>`/`FName`/`int32` only — `RegionPlace` is an `FName` (`:266`), so *"M8 P2 takes this struct as-is over the wire"* survives. `FSiegeAssistantRegionStatics` is a non-UObject static library with no members. The membership test runs inside `SelectUnitsForOrder`, on the authority. |
| **12** | **`SC-§20`** | ✅ Every suggested fix below is marked **HYPOTHESIS**. The implementer traces it before applying it. |

---

## 2. ⚖️ THE RULINGS — DECIDED EXPLICITLY, NOT BY SILENCE

### ⭐⭐ RULING 1 — THE TWO TRAILING-DEFAULT HAZARDS: **BOTH CLOSED. I RE-GREPPED BOTH MYSELF.**

**(a) `USiegeAssistantGrammar::Build` — the batch-killer TASK-548 found.** `grep "Grammar::Build("` across `Source/` returns **54 hits in 4 files**: 26 in `Tests/SiegeAssistantGrammarTest.cpp`, 26 in `Tests/SiegeAssistantSelectionTest.cpp`, **1 in `SiegeAssistantGrammar.cpp` (the definition, `:410`) and exactly 1 in `SiegeAssistantComponent.cpp`.** That one is `ComposeTurnGrammar` at **`SiegeAssistantComponent.cpp:3399`**, and it reads:

```cpp
return USiegeAssistantGrammar::Build(Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames(), Snapshot->GetRegionPlaceNames());
```

✅ **Three arguments. The `Plugins/` tree holds no call site at all** (its single hit is a prose line in a log string). `GetRegionPlaceNames()` returns `const TArray<FName>&` (`SiegeAssistantSnapshot.h:599`) and binds to the parameter. ⇒ **`{"in":…}` IS SAMPLABLE AT RUNTIME.** ⭐ **TASK-548's finding was real, was in scope (its own file; no other task could reach that line), and is the highest-value item in the batch** — without it, nine correct tasks would have shipped unreached and `AS-§21.11` outcome 5 would have made Stage 5 misdiagnose a missing function argument as the prompt under-teaching the shape.

**(b) `SiegeAssistantValidateSelection`'s fifth default.** `grep "SiegeAssistantValidateSelection("` across `Source/` + `Plugins/` returns **16 hits**: the declaration (`SiegeAssistantCommand.h:616`), the definition (`SiegeAssistantCommand.cpp:449`), **two whole-command call sites — `SiegeAssistantCommand.cpp:1095` and `SiegeAssistantComponent.cpp:1051`, and BOTH pass all five arguments** — and 12 arrays-only calls in `Tests/`, none of which holds an `FSiegeAssistantCommand`. `Plugins/` holds none. ✅ **`AS-§21.9`'s QA criterion is met.**

> ### ⚖️ AND THE BIGGER QUESTION THE DISPATCH ASKED, RULED: **YES. THIS NEEDS A STANDING LAW, AND THE EVIDENCE IS NOW TWO BATCHES DEEP.**
>
> **The measured pattern:** the ASSISTANT-EXCLUDE batch's blocker was a trailing-defaulted parameter; this batch's near-miss was a trailing-defaulted parameter; and **`AS-§21.9` had already named the cost in prose** (*"a caller that omits it validates NOTHING"*) — prose which did not prevent the second instance. ⚖️ **A hazard that has now fired twice under a law that describes it is not being controlled by description.**
>
> 📌 **PROPOSED, FOR THE MANAGER TO ADOPT INTO CONVENTIONS (I do not edit CONVENTIONS):**
> **THE TRAILING-DEFAULT LAW — a task that adds a trailing defaulted parameter to an existing function owes, IN ITS OWN HANDOFF, AN ENUMERATED AUDIT OF EVERY EXISTING CALL SITE OF THAT FUNCTION, produced by a grep whose exact command is pasted, with each hit classified as (i) updated, (ii) deliberately left at the default with the reason, or (iii) out of this task's ownership AND NAMED TO THE TASK THAT OWNS IT.** ⛔ *"Every existing call site stays byte-identical"* is the **benefit** of the shape, never the audit. ⚠️ **The audit is owed BY THE TASK THAT ADDS THE PARAMETER, even when it cannot edit the call sites** — TASK-546 could not have fixed `ComposeTurnGrammar`, but a two-line grep in its handoff would have *named* it, and the batch would not have depended on a downstream implementer's initiative.
> ⭐ **And the second leg, which is what makes it enforceable at a gate:** ⛔ **THE GATE RE-RUNS THAT GREP ITSELF AND PASTES ITS OWN COUNT.** TASK-549 states plainly it *cannot* test for a dropped `Build` argument (`ComposeTurnGrammar` is `private` and world-driven; the suite stays green either way) — **so the grep is the only mechanism, and a mechanism that lives only in a handoff is a mechanism nobody runs.**

---

### ⭐ RULING 2 — TASK-547's `ZONE = ` GENERATOR: **THE DEPARTURE IS RATIFIED. THE BOARD TEXT IS WRONG AND IS THE THING THAT MUST CHANGE.**

**Ruled FOR TASK-547, explicitly.** Board spec 4(b) says the line is *"GENERATED FROM `GetRegionPlaceNames()`"*. `GetRegionPlaceNames()` returns `RegionPlaceNames`, a `UPROPERTY(Transient)` member filled by `Capture()` (`SiegeAssistantSnapshot.cpp:515`) — **per-match state**, legitimately shorter on a map with one ancient ground. `BuildZoneA`'s own declaration contract says it *"READS NO MEMBER STATE, BY CONSTRUCTION"* and that making it state-dependent **is a QA FAIL**, because a varying Zone A destroys the measured KV prefix reuse **silently, as a latency regression rather than a wrong answer**. ⇒ ⛔ **THE BOARD'S WORDING, TAKEN LITERALLY, ASKS FOR THE QA FAIL.**

✅ **What shipped satisfies the actual requirement.** The line is generated at build time from `PlaceVocabulary`'s new `bHasRegion` column (`SiegeAssistantSnapshot.cpp:1088-1107`), which is a `static constexpr` table — **generated, never hard-coded** (`AS-§21.4`'s real demand: *"`PlaceVocabulary` stays the SINGLE OWNER and gains one `bHasRegion` column"*), and there is **no literal list of three anywhere in the file**. It mirrors the split the `places` block already ships: **Zone A prints the full fixed vocabulary; the grammar enforces what exists this match** — and the grammar *does* consume the live `GetRegionPlaceNames()` (RULING 1a), so a region the map lacks stays **unsamplable** even though the prompt names it. The `static_assert` at `:133` makes the empty case impossible.

⇒ 📌 **OWED: correct board item TASK-547 spec (4)(b) to read *"generated from `PlaceVocabulary`'s `bHasRegion` column"*, and add the state-freedom sentence, so the next reader is not led back into the trap.** ✅ TASK-549's strengthened `ZoneA.StaticPrefixContract` (byte-identical Zone A at 3 regions, at 1, and at 0) is the correct instrument and it **stands**; its declared coupling does not fire.

---

### RULING 3 — THE PLACEMENT CONTRADICTION: **I RULE THE REQUIREMENT. BOTH-BRANCH COVERAGE WINS; THE BOARD'S PLACEMENT CLAUSE IS SUPERSEDED — AND SO IS THIS GATE'S OWN CRITERION (4).**

The board says the `Kinds.Num()==0` branch; the dispatch demanded **both** branches. ⚖️ **They are contradictory, not merely different — a predicate inside the `all` branch cannot apply to the per-kind branch.** TASK-548 and TASK-549 both say so, and they are right.

**Ruled: the region filter belongs in the shared candidate gather** (`SiegeAssistantComponent.cpp:2190`), and my own spec's criterion (4), which quotes the board's placement, is **corrected here rather than enforced**. The reasons, in order:

1. ⚖️ **`SC-§21` is about the guard being where it can actually run.** In the shared gather it runs on **every** path that reaches a candidate; in the `all` branch it runs on one. `SC-§21` argues *for* this placement, not against it.
2. ⛔ **`AS-§21.6`'s own words settle it:** a region parsed and then dropped by a branch that never learned about it is the named automatic-FAIL. Writing the predicate where one branch cannot see it *builds that failure in* against the day a ruling opens region+selection.
3. ✅ **It costs nothing today.** The per-kind branch is unreachable with a region (parser refuses, validator refuses, backstop refuses) — so this is coverage for a future opening, not live behaviour, and the diff is one predicate rather than two.
4. ✅ **The stated constraints survive intact:** still ONE pass (`:2135`), two float compares per candidate, no new traversal, and sorting byte-untouched.

⇒ 📌 **OWED: correct board item TASK-548 spec (1)** to name the shared candidate gather. ⚠️ **`GetActorLocation()` is called twice per candidate** (predicate + the untouched `DistSq` line) — I **ratify** the choice: hoisting it would edit the shipped sort line and spend this batch's purely-additive safety property on a transform read. **NIT-7 records it so nobody "optimises" it into a diff at a compile gate.**

---

### RULING 4 — `AS-§21.5`'s SELF-CONTRADICTION: **THE SUPERSET IS RIGHT. THE LAW'S WORDING IS WHAT IS WRONG — AND THE JUSTIFICATION IN THE CODE IS ALSO WRONG, WHICH STRENGTHENS THE VERDICT.**

`AS-§21.5` enumerates `""` / `"all"` / `"none"` for `BadRegion` while justifying them as *"the three reserved wire symbols… can never name a place"* — and the third reserved symbol is **`now`**, not `""`. TASK-545 shipped `""` **plus all three** (`SiegeAssistantCommand.cpp:306-313`), so **every case the law enumerates behaves exactly as pinned and no pinned assertion can fail.** ✅ **RATIFIED.**

⭐ **AND I FOUND A REASON THE IMPLEMENTER'S OWN ARGUMENT UNDERSTATES: THE SUPERSET IS NOT MERELY HARMLESS, IT IS LOAD-BEARING.** `ParseRegionSymbol`'s doc comment (`:288-290`) justifies itself with *"`USiegeAssistantGrammar::Build` already refuses to emit any of the three as a generated place alternative"* — **that is false.** `CanonicalizeSymbols` drops `NAME_None`, empty, `all` and `none` (`SiegeAssistantGrammar.cpp:350-377) — it does **not** drop `now`. ⇒ **the parser's `now` clause is the ONLY guard against it**, not a redundant belt beside the grammar's braces. (No behaviour is at risk today: `PlaceVocabulary` cannot contain `now`.) **WARN-1** records the comment; the code stays.

⇒ 📌 **OWED: fix `AS-§21.5`'s bullet** to read *"`""` plus the three reserved wire symbols (`none` / `all` / `now`)"*, so the enumeration and the justification describe the same set. ⛔ **Do NOT delete the `now` clause from the code.**

---

### RULING 5 — THE VRAM TIER CHANGE: **ALL THREE PARTS RULED. IT SHIPS AS WRITTEN.**

**(a) THE CONSTANT'S VALUE — `151` STANDS.** The derivation yields **144 MiB exactly** (`36 × 8 × 128 × 2 × 2 × 1024 B = 150,994,944 B`); `151` is that same quantity in **decimal MB**, and the constant is named `…MiB`. ⚖️ **A unit mismatch is normally a defect; here it is ~7 MiB in the direction `AS-§19` mandates** — a gate that **promotes** must assume the **larger** cost — **and the source states the discrepancy, the reason and the alternative in full** (`SiegeLlamaSubsystem.cpp:132-137`). ⇒ **RATIFIED. `144` is the *less* safe number and must not be "corrected" to.** ⛔ TASK-543 was right to flag it rather than reconcile it silently.

**(b) THE DEMOTION BAND — IT SHIPS, AND IT SHIPS *NAMED*.** Re-derived independently: Full `3471 → 3622`, Partial `2032 → 2183`, both `+151`. ⇒ **3471–3621 MiB: Full → Partial** (measured 0 aborts of 5; degraded speed, still correct) and ⛔ **2032–2182 MiB: Partial → CpuOnly, a tier that MEASURABLY FAILS (2 of 5 generations hit the ceiling).** ⚖️ **Ruled to ship, on three grounds:** the alternative is **promoting** a machine into a tier it cannot run, which is `FT-§6`'s already-named defect and strictly worse; **no recorded machine is in either band** (6893 MiB → Full with +3271 margin; 3038 MiB → Partial with +855); and the CPU fall-through defect is **fenced out of this batch by ruling**, so fixing it here would be the scope breach. ⛔ **But it is a real cost of Jonathan's ruling 2 and it is NOT to be reported as "no change":** it goes to him with the checkpoint, and **TASK-552(D)'s live `vram_delta` is what retires the whole constant.** **WARN-5.**

**(c) THE ~2× OVER-RESERVATION ON THE PARTIAL GATE — SHIPS, UNCHANGED.** At `PartialGpuLayers = 18 of 36` only the offloaded half of the KV cache is GPU-resident, so the layer-scaled figure is ~half. ⚖️ **Scaling it would stack a second derived number on the first, which is not more knowledge — it is more assumption**, and it would move the gate in the **unsafe** direction on a promoting comparison. The ruling said *add it to BOTH*; the source says it is conservative **by choice** and must not be "corrected" by scaling (`:139-145`). ⇒ **RATIFIED.** The honest resolution is a **measurement**, not a second derivation, and it is TASK-552's.

**(d) THE ONE JUDGMENT CALL INSIDE THE FENCE — `OutReason`'s CpuOnly threshold number: RATIFIED.** Control flow, condition and returned tier are byte-unchanged (`:957-966`); only the *printed* threshold followed the partial gate. ⚖️ **A selector whose reason string prints a threshold the code never tested against is a log that lies quietly** — spec item (3) required the strings to name the new term, and this is that requirement, not the fenced defect (which is about the tier *selected*, not the number *reported*).

---

### RULING 6 — THE THREE SMALLER DECLARED DEPARTURES

- ⚠️ **TASK-549's edit to `Tests/SiegeAssistantGrammarTest.cpp` — ✅ RATIFIED, NOT REVERTED.** It is inside the dispatched `Tests/` scope, TASK-546's **D4** assigns it by name, and I read the diff's shape: **purely additive** — one helper (`MidMatchRegionPlaces()`, `:81-84`), four fixture rows (`:423-426`), one reached-the-new-rules guard (`:462-469`), and the repair of a comment that had become **false** (*"every shape the builder can produce"* — four fixtures all using the 2-arg overload). ⛔ **No existing assertion changed.** ⚖️ **Reverting would leave a false claim shipped inside the one test guarding a charset this project has already been bitten by twice** (`at_least` cost TASK-413 two of six bars; `except_list` was caught pre-ship). ⇒ ⛔ **CONSEQUENCE FOR TASK-551, AND IT IS NOT OPTIONAL: the file is NOT in the board's commit-path list. See WARN-4.**
- ✅ **TASK-544's `struct` — RATIFIED. Do not "harmonise" it.** The `AS-§21.9` pin says `struct` and the `names:` block independently says `struct`; two sources agree and the pin is the cross-task contract. The 4/4 house precedent (`class { public: … }`) is a style observation, and TASK-544's own reasoning checks out: MSVC `C4099` needs a *conflicting* declaration and there is none (the type is forward-declared nowhere), and the class-key does not change the mangled symbol of a static member function. **Recorded here so nobody edits the pin to match the precedent, or the header to match the pin's siblings.**
- ✅ **TASK-541's second hunk (the `// --- INTENTS ---` comment block) — RATIFIED.** The board's spec item (2) required it, it is the `SC-§22` second home of the false claim, it is inside that task's exclusively-owned file, and it costs **zero prompt bytes** (I confirmed it is a C++ comment, not a `TEXT()` payload). ⚖️ **The board is the spec of record and the dispatch is its summary** — the programmer resolved that correctly.

---

## 3. FINDINGS

⚠️ **Every suggested fix below is a HYPOTHESIS (`SC-§20`), not a patch. The implementer traces it before applying it.**

### BLOCKER — **none.**

### WARN

- **[WARN-1]** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantCommand.cpp:288-290` — **a false claim in the comment that justifies a shipped decision.** `ParseRegionSymbol`'s doc says *"`USiegeAssistantGrammar::Build` already refuses to emit any of the three as a generated place alternative, so none of them can ever name a real region."* At the artifact, `CanonicalizeSymbols` (`SiegeAssistantGrammar.cpp:371`) drops **`all` and `none` only** — **`now` is not dropped.** No behaviour is wrong (the parser refuses `now`, and `PlaceVocabulary` cannot contain it), but the sentence understates its own guard and is the `SC-§22` shape this batch's diagnosis is literally about. **HYPOTHESIS:** replace the final clause with *"`Build` drops `all`/`none` before they can become `zone` alternatives; `now` is NOT dropped there, so this check is the only guard against it."* Cost: one sentence, zero emitted bytes.
- **[WARN-2]** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantGrammar.cpp:265` — *"It runs Build over **all four builder shapes**"*. False since `Build` grew a third parameter; the test now runs **eight**. Flagged by TASK-549, who could not edit this file. **HYPOTHESIS:** `four` → `eight`. Zero emitted bytes.
- **[WARN-3]** `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp:~976` (located by symbol: the `BuildZoneA` header block, the sentence containing *"RE-BASED BY TASK-523 against 5424"*) — stale in the present tense; the sentence **two lines below it** states the current fact (5658, TASK-549). Self-contradiction two lines apart. **HYPOTHESIS:** past-tense it (*"were re-based by TASK-523 against 5424, and are re-based again by TASK-549 against 5658"*). Zero emitted bytes.
- **[WARN-4]** ⛔ **`.claude/pipeline/TASKBOARD.md` → TASK-551 spec item (5): the explicit commit-path list OMITS `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp`.** That file **is** edited (RULING 6, ratified), and item (5) mandates commit **by explicit file path only, never a directory pathspec**. ⇒ **As written, the commit would leave a ratified, gated edit uncommitted and the tree dirty after the "one commit".** ⛔ **This is the only finding that can bite TASK-551 mechanically.** **HYPOTHESIS:** add the path to item (5)'s list. ⚠️ **Build-master must reconcile its own `git status` against the list before committing rather than trusting the list** (`SC-§9`).
- **[WARN-5]** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:938-955` — **the 2032–2182 MiB demotion band ships (RULING 5b).** Not a code defect; **a consequence that must reach Jonathan before he plays** and must be discharged by TASK-552(D)'s live reading. ⛔ Do not silently "improve" the constant to close it.
- **[WARN-6]** `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h:266-267` — `SafetyMarginTokens`' sanity paragraph still reads *"= 1683 **of 2048**"* and *"1933 **of 2048**"*. **TASK-543's refusal to edit it is RATIFIED:** the block is inside its scope fence, and its line rests on the **stale `1139`** that `AS-§21.7` forbids recomputing — rewriting it now would re-bless a stale operand while making it look freshly derived. ⇒ 📌 **Board a comment-only follow-up for AFTER TASK-552 prints the real `zoneA_tok`.** ⛔ **Not owed before this commit.**

### NIT

- **[NIT-1]** `CONVENTIONS.md` `AS-§21.5` — the `bad_region` bullet enumerates a set its own justification contradicts (RULING 4). Manager owns the fix.
- **[NIT-2]** `CONVENTIONS.md:2864` (`AS-§21.2`) — presents *"NO PROMPT-LEVEL TASK MAY BE JUSTIFIED BY NAMING THE ROWS IT WILL FIX"* **inside quote marks**; the artifact at `:1208` reads *"…IT BINDS EVERY FUTURE PROMPT-LEVEL TASK ON THIS MODEL: **NO SUCH TASK** MAY BE JUSTIFIED…"*. **The line range is correct and the substance is identical — it is a compression presented as verbatim.** Raised by TASK-541 and confirmed by me at both artifacts. **HYPOTHESIS:** quote it verbatim, or drop the quote marks. ⚠️ Nothing turns on it; recorded because this batch is strict about quote fidelity and citations have drifted.
- **[NIT-3]** `SiegeAssistantCommand.h:266` / `AS-§21.9` / three handoffs call `RegionPlace` **"the seventh field"**. It is the **eighth `UPROPERTY` member** (`Intent`, `Kinds`, `Counts`, `Where`, `TriggerKind`, `TriggerAtLeast`, `ExcludeKinds`, `RegionPlace`). The ordinal is inherited from the pin's *"the SIX shipped fields"*, so the correction belongs in `AS-§21.9` first. Zero consequence — **the placement claim that matters (appended last, none of the prior members moved) is TRUE and verified.**
- **[NIT-4]** **TASK-543 §7 routed a stale comment that does not exist.** `Tests/SiegeAssistantZoneATest.cpp:636` is claimed to read *"`ContextTokens` frozen at 2048"*; grep for `2048` across `Source/GitClaudeUnrealTest/Siegebound/Tests/` returns **zero hits in any file**. Either the citation drifted or TASK-549's re-base removed the line. ⇒ ✅ **NOTHING IS OWED. Do not go looking for it.**
- **[NIT-5]** TASK-545 §4 flags `SiegeAssistantComponent.cpp:1039` as an open 4-arg call site. It is `:1051` post-TASK-542 and it is **closed** (RULING 1b). Line-number drift in a correctly-routed finding; recorded so a later reader does not re-open it.
- **[NIT-6]** `ContextGrowthVramReserveMiB` is named `MiB` and valued in decimal MB. **Ratified (RULING 5a)**; the source documents it. Recorded so the mismatch is a known, deliberate one when TASK-552 replaces the constant.
- **[NIT-7]** `SiegeAssistantComponent.cpp:2190` + `:2198` — `GetActorLocation()` is read twice per candidate. **Ratified**: hoisting it would edit the shipped sort line and spend this batch's additive-diff property on a transform read. ⛔ **Not a compile-gate optimisation.** Also recorded: TASK-549's Zone-C control clears `PlaceLocations` as well as the region arrays, which makes the byte-equality assertion **stronger**, not weaker — noted so nobody reads the control as mismatched.

---

## 4. ⭐ THE SPECIFIC VERIFICATIONS THE DISPATCH DEMANDED — ONE LINE EACH

| demand | result |
|---|---|
| ⭐⭐ **Additivity, byte-identical** | ✅ §1 item 1. **One** intentional change, to a currently-*failing* input, refused on both sides. |
| ⭐⭐ **Zone C byte-unchanged, against the SHIPPED builder** | ✅ `Selection.ZoneCIsByteUnchangedByRegions` runs the real `BuildZoneC` at 13 kinds, `PerKindTotal` **9 and 12**, `TestEqualSensitive`, with a **vacuity precondition** (the region snapshot must really have published 3, else it skips loudly) plus coordinate-absence byte assertions. ⚠️ **Its honest limit is stated by its author and I confirm it:** it proves invariance to region state, not before/after the batch — which is the claim that needed guarding, and the only one obtainable once the batch is on disk. |
| **Zone A arithmetic, component by component** | ✅ Re-counted by hand: 16 / 76 / 147 / −5 all reproduce; `5116+44+111+153−5+16+76+147 = 5658`; **spike lane stays 5116**; fixture never re-copied (asserted three ways). |
| ⛔ **`AS-§12f` clean sweep** | ✅ Independently re-swept. Clean in code, comments, all nine handoffs, and this report. |
| ⛔ **Region refused, never dropped** | ✅ Parser (3 army-wide verbs) · validator (wire side) · executor backstop · fail-closed resolve · loud empty-after-region refusal **through the existing `AskUnsupported` outcome** (traced: `return false` → `ExecuteZoneOrder`/`ExecuteFollowOrder` → `ExecuteAndReport` → `PushMessage(AskUnsupported)`, `:1676`). ⛔ **No new ask symbol, no new reason code, no new template row, no silent no-op.** |
| ⭐ **`DescribeCommandForPlayer` renders the region** | ✅ `:1346-1363` — `Send all in ancient_ground_near (enemy_castle)`. Built into `{Selection}`, no new frame, no new template row, raw symbol, `SC-§31` honoured (it describes what will run, including a region that resolves to nobody). **No-region renderings are byte-identical** (the clause is inside the `!= NAME_None` guard). |
| **Struct stays `uint8`/`int32`/`FName`** | ✅ Verified member by member; `ESiegeAssistantIntent : uint8`. |
| **Rule-name charset** | ✅ `inplace` / `zone` — `[a-z]` only, no underscore, asserted in **two** tests over eight grammar shapes. |
| ⚠️ **Degenerate extents, two ways — RECONCILED** | ✅ **They are LAYERED, not contradictory, and both fail closed.** TASK-547's publishing filter (`SiegeAssistantSnapshot.cpp:511-513`, `> 0.f` on both axes) makes a `(0,0)` region **unsayable** — never published, so never in the grammar and never resolvable. TASK-544's un-guarded predicate is the **downstream** answer: if a degenerate box ever reached it, it admits nobody. ⇒ **RATIFIED, both.** ⛔ Adding a guard to `IsPointInRegion` would fork it from the two shipped donors — the exact divergence `AS-§21.4` forbids — and **would have to be applied to `AAncientGround`/`ACaptureZone` too**, which are `NOT touched` files. |
| `TestEqualSensitive` not `TestEqual` (`SC-§13`) | ✅ Every `TestEqual(` in the batch's new test code compares **integers**. Every string/byte claim uses `TestEqualSensitive`. Checked by grep across both test files. |
| 🔒 `holdout2` untouched | ⚠️ **Content-level: no `Docs/Data/*.csv` is read or written by any file in this batch, and TASK-549 declares it unopened.** ⛔ **The diff-level proof is TASK-551's and is listed in §6.** |
| ⚠️ Line numbers | ⛔ **Every symbol in this report was located by grep. No cited line — including the dispatch's and the handoffs' — was trusted.** |

---

## 5. ⛔ WHAT A PASS HERE DOES **NOT** MEAN (stated so it is not inferred)

- ⛔ **Nothing compiled.** UBT has not seen one line of this batch. TASK-551 is the first compiler.
- ⛔ **No test has ever run.** 28 `Selection.*` tests and 4 re-based `ZoneA.*` tests **compile-in-principle**; compiling is not passing.
- ⛔ **The executor's region filter is UNTESTED and cannot be tested here** — `SelectUnitsForOrder` walks a `UWorld`. The predicate and the geometry lookup are covered; **the loop that joins them, the fail-closed refusal, the empty-after-region refusal and the arithmetic in the logs are five `UE_LOG` lines plus Jonathan at TASK-552.**
- ⛔ **Nothing here parses the grammar with llama.cpp.** Only the target parser can prove a grammar valid.
- ⛔ **A future edit that drops `Build`'s third argument would compile, link, run and leave the suite GREEN.** RULING 1's grep is the only mechanism, and that is why it is proposed as a standing law.
- ⛔ **Not promised: that the model reliably emits either shape** (`AS-§21.11` #5). This gate rules on source, not on a model nobody has observed.

---

## 6. NOTES FOR BUILD-MASTER — TASK-551

**⛔ WHAT TO WATCH AT THE COMPILE (in order of likelihood):**

1. ⛔ **The two trailing-default call sites are the batch's whole feature.** If you author *any* compile fix near `SiegeAssistantComponent.cpp:1051` or `:3399`, **re-run both greps in RULING 1 afterwards** and paste the counts. A fix that drops an argument compiles clean and disables the feature silently.
2. **`static const` → `static constexpr` on `PlaceVocabulary`** (`SiegeAssistantSnapshot.cpp:88`) with a `constexpr` counter and two `static_assert`s. Every initialiser is a string literal or a bool, so it should be a constant expression — **but this is the batch's only constexpr-evaluation bet, and a failure here is loud and immediate.**
3. **`FSiegeAssistantRegionStatics`** is the batch's only new TU pair. `GITCLAUDEUNREALTEST_API`, `struct` (ratified), no `.generated.h`, no UHT participation, no `Build.cs` change. A link error naming `IsPointInRegion` means the `.cpp` did not reach the module.
4. **Format-string arity** in `ChooseTier`'s three `OutReason` strings (5/6/5) and the new `UE_LOG`s in `SelectUnitsForOrder`. I checked them by hand; the compiler checks them properly.
5. ⚠️ **A failure naming `SiegeLlamaSpike.cpp` is FOREIGN (D4).** Route it to TASK-481 as early information; ⛔ never count it against this lane or its QA-loop budget.
6. ⚠️ **A ~2 s death with `0x800711C7` is Smart App Control, not a code error.** Jonathan-only fix; ⛔ must not spend a QA loop.
7. ⛔ **Parse the log for `Result: Failed`. NEVER trust `$LASTEXITCODE`.**

**⛔ WHAT TO WATCH IN THE RUN:**

8. **Expect RED→GREEN on exactly two shipped tests** — `ZoneA.MeasuredCharCount` and `ZoneA.TwoLaneByteEquality` were red by design and are re-based to **5658**. ⛔ **If either fails now, the failure names its component and its task number — report the component, do not "update it to pass."**
9. ⭐ **If `ZoneA.TwoLaneByteEquality` reports *"the declared D4+D5 diff no longer applies to the frozen spike fixture"*, STOP.** ⛔ **Do not re-copy the builder's output into the fixture** — that destroys the only surviving record of the bytes `5116` was measured on.
10. **Baseline for the delta: 73 tests green at TASK-538.** This batch adds **15** new `Selection.*` tests. ⛔ **State the total you actually observe and every failure by name. "As expected" is not a result.**
11. ⛔ **DO NOT RUN `DumpAssistantPrompt` / `SpikeEval` / `SpikePrompt`.** `ReportFirstCapture` is a **one-shot latch** and spending it here destroys TASK-552's `zoneB_chars` reading — TASK-528's entire blocking precondition.

**⛔ PRE-COMMIT — THE FOUR CLAIMS ONLY A DIFF CAN DISCHARGE (I have no Git tool by design):**

12. 🔒 **`Docs/Data/assistant_eval_holdout2.csv` is byte-untouched**, and **no** `Docs/Data/*.csv` appears in the diff at all.
13. **No `.umap`** in the diff — 🔒 `Content/Maps/L_Arena.umap` **SHA256 before and after, hash never mtime**.
14. **No `Build.cs`**, no `Content/` asset, no binary, no `.gguf`, no `Models/` path.
15. ⛔ **`Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` is absent from the diff (D4).** ✅ I confirmed this at the *content* level (no `inplace`, no `ZONE   = `, no `{"in":`, with a positive control per `SC-§14`) — **the diff is your half.**
16. ⛔⭐ **ADD `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` TO THE COMMIT-PATH LIST — see WARN-4.** It is a ratified edit and the board's list omits it. **Reconcile your own `git status` against the list; do not trust the list.**
17. ✅ **COVERAGE CHECK (`SC-§29`): all nine of 541–549 name TASK-550 in their handoffs and their board entries. I confirmed it. The gate file is `qa/TASK-550.md` and there is no other.**
18. ⚠️ **If you author a compile fix, that diff is CODE and owes its own `SC-§27` diff-scoped verdict before the commit** — changed lines plus the fences the fix must not have disturbed. ⛔ Not a re-litigation of this passed design.

---

## 7. ⭐ EVIDENCE TASK-552 MUST CAPTURE (beyond its own spec)

1. ⭐⭐ **The raw model output for BOTH sentences, verbatim** — `LogSiegeAssistant`, `Turn %d: RAW MODEL OUTPUT (%d chars): [...]`. **This is the first time that JSON exists as an artifact**, and it either CONFIRMS or FALSIFIES `AS-§21.1`. ⚖️ **Both outcomes are results and both are reported as results.**
2. ⭐ **The selector's region lines, whichever fires:** `Selector applied a REGION: %d eligible, %d outside region '%s', %d INSIDE…` · `Selector refused - the region emptied the selection…` · `Selector refused - region '%s' did not resolve at execution time…`. ⛔ **These five log lines are the ONLY evidence the filter actually filtered** — no automation test can reach them.
3. ⛔ **If `Selector refused - a command reached the selector naming region … AND a …selection/exclusion` appears at `Warning`, that is a CODE OR WIRE DEFECT, not a model error.** Report it immediately; two gates should have refused it first.
4. **`vram_delta` on the load line at `ctx=3072`** — it settles the 128-head-dim assumption and **replaces `ContextGrowthVramReserveMiB`** (RULING 5). Capture the `tier=` and the `MiB free` figure with it, so the demotion bands can be re-derived against a measurement instead of a derivation.
5. **The three unprinted numbers** — `[budget] STATIC PREFIX REGISTERED: <chars> -> <tokens>` (the real `zoneA_tok`, which is what ends `AS-§12g`'s standing WARN), `[budget] ASSERTION PASSED … Slack=<N>` (and **whether that assertion has ever executed at all** — `SC-§32`), and `FIRST LIVE CAPTURE … zoneB_chars=<N>`. ⚠️ **`chars` should read 5658.** If it does not, this gate's arithmetic is wrong and that is the finding.
6. ⚠️ **The watch list is `AS-§21.11` unchanged** — `region_conflict:fallback`, the Standard-profile-only zone orders, the three selectable regions, the loud empty-region refusal, and ⛔ **no promise that the model reliably emits the fifth shape.** ⛔ None of the five is a bug.

---

## 8. 📌 BOARD STATUS — I HAVE NO PARTIAL-EDIT TOOL; PLEASE PROXY

⚠️ **`TASKBOARD.md` is 2.7 MB and I hold no partial-edit tool — I will not rewrite it wholesale.** Requested flips, for the orchestrator to proxy:

- **TASK-541 · 542 · 543 · 544 · 545 · 546 · 547 · 548 · 549** → **`qa-passed`** / `ready-for-integration` (gate `qa/TASK-550.md`)
- **TASK-550** → **`done`** (verdict PASS, 0 blockers)
- **TASK-551** → dispatchable, with **WARN-4's commit-path correction applied first**
- 📌 **Board-text corrections owed (RULINGS 2 and 3):** TASK-547 spec **(4)(b)** and TASK-548 spec **(1)**
- 📌 **CONVENTIONS corrections owed to the manager (NIT-1, NIT-2, NIT-3) and one proposed standing law (RULING 1)** — ⛔ I do not edit CONVENTIONS.
