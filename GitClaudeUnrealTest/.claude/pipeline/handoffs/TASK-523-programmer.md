# TASK-523 — ⭐ THE MEASUREMENT: `SiegeAssistantSelectionTest.cpp` + the Zone-A tests re-based

**Status:** `ready-for-qa` · **QA gate: TASK-525** (`.claude/pipeline/qa/TASK-525.md`)
**⛔ NOT compiled and NOT run. TASK-526 owns the only compile and the only suite run.**

**M8 DECLARATION (verbatim, as required):**
> *"adds no replicated property, no new replicated class, no new relevancy tier."*

---

## 1. FILES TOUCHED — three, and the third is one the `names:` block does not list

| file | what happened |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantSelectionTest.cpp` | **NEW.** 13 tests under `Siegebound.Assistant.Selection.*`. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantZoneATest.cpp` | **RE-BASED.** `TwoLaneByteEquality` re-purposed, `MeasuredCharCount` re-based to 5424/5116, **and a THIRD test nobody listed** — see §5. |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantGrammarTest.cpp` | **ONE assertion re-based**, `Grammar.SelectionCap`'s `who` count 3 → 4, plus one added `except` reference check. ⚠️ **Not in my `names:` block; assigned to me by TASK-518's board entry item (b).** See §7-D2. |

⛔ **No shipped `.cpp`/`.h` was edited. No corpus file was opened or written. `Plugins/SiegeLlama/**` untouched. `Docs/Data/assistant_eval_holdout2.csv` NOT opened, read, grepped or quoted. No `Build.cs` change. No compile, no test run, no Git, no editor/MCP/PIE.**

---

## 2. ⭐ EVERY TEST, BY NAME, AND WHAT EACH WOULD CATCH

### The roster / Sorcerer instruments — ⭐ the reason this task exists

| # | test | what it would catch |
|---|---|---|
| 1 | **`Siegebound.Assistant.Selection.RosterShowsAllThirteenKinds`** ⭐ | **Jonathan's defect #2, directly.** A 13-kind board must print the literal `sorcerer`, all 13 rows in DT_Cards order, and `other_kinds: none`. Catches: `MaxRosterKinds` silently reverting to 8; the roster losing its fixed row order; the Sorcerer row printing without its counts. Also pins `MaxRosterKinds == 13` and that `sorcerer` is the LAST card row (the mechanism). |
| 2 | **`Siegebound.Assistant.Selection.CollapseNamesTheHiddenKinds`** ⭐⭐ | **The highest-value test in the batch, and the leg NO other instrument reaches.** Forces a collapse (240-byte `order:` + 240-byte `pending:`) and asserts the collapsed kinds appear **BY NAME**, that `sorcerer` is named and is LAST (row order preserved), that printed + collapsed == the whole board, that the **retired counts-only format `N kinds, M units` is GONE**, and that the `(<N> units)` aggregate survives. ⛔ Has an explicit **pre-condition assertion** that a collapse actually happened — without it the test could pass on the uncollapsed path and report SAFE about a path that never ran. Also **asserts (Occurrences == 0) that the shipped `Snapshot roster TRUNCATED` Warning FIRED**, so a collapse that degrades the prompt silently (the TASK-419 WARN-5 defect) fails here. |
| 3 | **`Siegebound.Assistant.Selection.FixedKeyOrderSurvives`** | The fixed-key law across **three** board shapes (uncollapsed / collapsed / empty): every key emitted, `other_kinds:` **always** emitted including on an empty roster, and the keys in **order** (`[FORCES] places roster other_kinds stances hero pending [ORDER] order`). Catches a future *"only print `other_kinds` when something collapsed"* tidy-up, and a shuffled key set that a presence-only check would pass. |
| 4 | **`Siegebound.Assistant.Selection.ShrinkLoopNeverHidesASymbol`** ⭐ | Test 2 measures **one** collapse; this measures **all of them**. Sweeps `order:` from 0 to `MaxUtteranceBytes` in 4-char steps, on **two** board magnitudes, asserting at every rung: (a) printed + collapsed == 13 — **no symbol ever vanishes**; (b) every one of the 13 symbols is visible as a row **or** by name; (c) the shrink loop is **monotonic** (a longer sentence can never widen the roster — the one property TASK-517's new format could have broken); (d) `sorcerer` present at every rung. Would catch a format that names collapsed kinds correctly at depth 1 and drops one at depth 9. |

### Parser — the exclusion accept/refuse matrix

| # | test | what it would catch |
|---|---|---|
| 5 | **`…Selection.ExclusionParses`** | `{"all_except":["miner"]}` → `ExcludeKinds == [miner]`, **`Kinds` EMPTY**. Arities 1/2/3 accepted on all four selection-bearing verbs. ⭐ The empty-`Kinds` assertion is the seam the executor keys on **and** the premise of TASK-524's `DEV-26`: if the parser ever synthesised an enumeration, the exclusion would silently become a positive selection capped at 3. Pins `SiegeAssistantMaxExclusionKinds == 3` and its mirroring of the selection cap. |
| 6 | **`…Selection.ExclusionArityRefused`** | 0, 4 and 5 kinds → `exclude_arity`, command fully reset. ⛔ Includes the assertion that **`{"all_except":[]}` does NOT parse to the same command as `who:"all"`** — i.e. the empty list is REFUSED, never promoted. That is the exact "quietly promote and drop the exception" move a future simplification would make. |
| 7 | **`…Selection.ExclusionConflictRefused`** | selection + exclusion → `exclude_conflict` (through the **validator**, because the JSON shapes are disjoint — see §6); repeated symbol → `duplicate_kind`; a repeat differing only in **case** still refused; and `who:"none"` still behaving exactly as before. |
| 8 | **`…Selection.ExclusionRefusedOnArmyWideIntents`** ⭐ | **The ruling-3 assertion.** `charge`/`fallback`/`rally` + exclusion → `exclude_conflict`, payload **names the verb**, and ⛔ the command is **fully reset** — which is what distinguishes *"refused"* from *"accepted with the exception quietly discarded"*. Also pins `SiegeAssistantIntentTakesSelection` as the gate (no second army-wide list), and includes the control that the same exception on `send` is **accepted**. This is the test that stops *"fall back except the miners"* becoming *"fall back INCLUDING the miners"*. |
| 9 | **`…Selection.ExclusionSymbolsRefused`** | 8 malformed payloads by reason code: `bad_type:all_except` (string / object), ⛔ **the DECLINED count-controlled item `{"kind":…,"n":5}` → `bad_type`**, `bad_kind` (`none` / empty / whitespace), `unknown_key` (extra key, misspelled key). Plus: mixed-case symbols normalise to lower case rather than being refused. |
| 10 | **`…Selection.PreExistingWhoShapesUnchanged`** | ⭐ **The additivity claim, measured as BEHAVIOUR (`SC-§18`), not as diff arithmetic.** All three pre-existing `who` shapes still parse identically and leave `ExcludeKinds` empty; the six pre-existing refusals (`bad_who`, `who_arity` ×2, `who_required`, `duplicate_kind`, `bad_type`) still report **their own codes and not the new ones** — a widened code would silently re-route a familiar failure to a wrong clarification with no compiler diagnostic. ⛔ Also asserts `except` as a **fourth top-level key** is refused (`unknown_key`), pinning AS-§20.1's rejected alternative. |

### The trailing-default hazard, the grammar, and the accept key

| # | test | what it would catch |
|---|---|---|
| 11 | **`…Selection.ValidatorExclusionArgumentIsLoadBearing`** ⭐ | **The batch's highest-risk line.** For three commands invalid *because of their exclusion alone*, it calls `SiegeAssistantValidateSelection` **both ways** and asserts the two calls **DISAGREE** — the 4-arg call refuses, the defaulted 3-arg call reports VALID. That is the exact delta `SiegeAssistantComponent.cpp`'s call site buys. Fires if anyone changes what the default means (makes the parameter required, or gives it a non-empty default). It also asserts the converse — a valid exclusion-free selection passes both forms identically — so the "strictly additive" claim holds for the arrays-only callers. ⚠️ **See §6 for what this test honestly cannot do.** |
| 12 | **`…Selection.GrammarAdmitsExceptOnlyWithKinds`** | Read off **the real emitter**, not TASK-518's hand-transcribed handoff block. Populated roster: `except`/`exceptlist` defined, `who` has **4** alternatives in the grammar's own order (`selection` then `except`), ⛔ `except_list` (underscore) is **not** a rule name, `all_except` appears only inside a terminal, `exceptlist` has exactly 1/2/3 **bare `kind`** references and **zero** `item` references, ⛔ **no `\"n\"` key anywhere in the exclusion rules**, no `*`/`+`/`?` repetition operators, and the grammar is byte-deterministic. One-kind roster: all three arities still emitted (bounded by the CAP, not the board) and the parser refuses the degenerate repeat. Empty roster: `except`/`exceptlist`/`kind` **omitted**, `who` back to **2**. |
| 13 | **`…Selection.PositionalAcceptKeyResolves`** | `GetPositionalKey(EKeys::Z)`: **identity on a miss**, **translation on a hit** (Dvorak → Semicolon, and the direction is asserted), identity for a key absent from a *populated* map, and ⛔ **NEVER `EKeys::Invalid`** — the exact value `TMap::FindRef` returns on the most common path (empty map, QWERTY host), which is TASK-516's flagged "obvious simplification". Then sweeps **all 26 shipped letter positions** asserting neither `EKeys::Invalid` nor ⛔ **`EKeys::Escape`** is ever produced. |

---

## 3. ⭐ WHICH TESTS WOULD HAVE CAUGHT JONATHAN'S TWO REPORTED DEFECTS

**Defect: *"whenever I say all units, it doesn't seem to include sorcerers even when they were spawned."***
- **Test 1 `RosterShowsAllThirteenKinds`** catches the cap leg (an 8-kind cap prints 8 rows and `sorcerer` never appears as a row).
- **Test 2 `CollapseNamesTheHiddenKinds`** catches the **root cause** — the counts-only collapse line. Against the shipped-at-report-time builder it fails on three independent assertions at once: `sorcerer` absent from Zone C, `sorcerer` absent from the collapse line, and the retired `N kinds, M units` string present.
- **Test 4 `ShrinkLoopNeverHidesASymbol`** catches it at **every** sentence length rather than at one, which matters because the defect is length-dependent — that is exactly why it read as intermittent.
- ⛔ **TASK-524 is right that the corpus cannot close this** (its findings F-1 and F-5): the correct answer is `who:"all"` → an **empty** kinds array, and an empty cell means *"not asserted"*; and the eval lane hardcodes fixture `t0`, whose roster prints all 13 kinds **unconditionally**, so **the collapse has never happened on that lane**. A green `DEV-31` is evidence about the *comprehension* leg only. **Tests 2 and 4 are the collapse leg, and they are the only instruments for it.**

**Defect: *"send all units except for some… it doesn't seem to handle that very well."***
- **Tests 5–10** are the whole accept/refuse matrix. The one that closes the dangerous half is **Test 8**: without it, an exception on `fallback` could be parsed and then dropped, producing a valid-shaped wrong command that looks obeyed.
- **Test 12** proves the model can *reach* the shape at all (grammar) and that the declined count-controlled variant is **inexpressible**, not merely unimplemented.

---

## 4. THE RE-BASED NUMBERS — ⛔ RE-COUNTED, NOT TAKEN ON TRUST

| figure | value | provenance |
|---|---|---|
| **SHIPPED Zone A** | **5424** chars / **5424** UTF-8 bytes | ⛔ **Independently re-derived by me**, not copied from TASK-521's handoff. |
| **SPIKE lane Zone A** | **5116** — ⛔ **UNCHANGED** | `Plugins/SiegeLlama/**` deliberately not touched (`D4`, `FT-§16`, TASK-481 mid-flight). |
| **`D4` delta** | **+308** | `+44` `WHO =` schema line · `+111` rule 1 · `+153` rule 2. |
| **AS-§20.4 budget** | 325 chars for the whole batch | **17 chars of slack remain**, asserted in the test. |
| `zoneA_tok = 1139`, `77.1 %` KV reuse | ⛔ **STALE — PENDING RE-MEASUREMENT ON THE MODEL** | ⛔ **Not recomputed by arithmetic, not deleted.** Only `Siege.Llama.SpikePrompt` prints them. |

**How I verified 5424 without a compiler or a model** (chars are countable offline — `AS-§20.4`'s own rule): I extracted every `Out += TEXT(...)` literal from the shipped `BuildZoneA` and from the frozen spike fixture, confirmed the five `D4_*` literals in my test file are **byte-exact copies** of the real ones (4 from the shipped builder, 1 from the frozen fixture, and the anchor present verbatim in **both**), and measured the deltas: **44 + 111 + 153 = 308 → 5116 + 308 = 5424.** The rule-line insertion point was confirmed against the shipped source order (anchor → rule 1 → rule 2). **The test itself re-checks all three component lengths at run time**, so a transcription error fails with the component named rather than as an unexplained total.

### ⭐ HOW `TwoLaneByteEquality` WAS RE-BASED — and why this is not a re-copy

The board forbids the obvious fix, and the test's own failure message forbids it too. So the test was **re-purposed, not silenced**:

> It builds the **frozen spike fixture**, applies **exactly the three declared `D4` edits** to it — each one a separate literal in the file, measured by the compiler — and asserts the result is **byte-identical to the shipped `BuildZoneA`**.

- ⛔ The frozen fixture is **untouched**; ⛔ `BuildZoneA`'s output is **nowhere** copied into it.
- ⭐ This is **strictly stronger** than the byte-equality it replaces. Equality only said *"the two lanes agree"*. This says *"the two lanes differ by precisely the diff the manager ruled, and by nothing else"* — a **fourth, undeclared** Zone-A edit still fails it, loudly, with a character offset and a window.
- It **guards its own derivation**: if fewer than three edits match the fixture, the test errors out *before* comparing, because a `Replace` that matched nothing would silently degrade the assertion back into the byte-equality that has been ruled false.
- It **also asserts the divergence is still real** (`TestNotEqualSensitive` shipped vs spike). If the two lanes ever became equal again, either the spike was edited (⛔ another batch's instrument) or TASK-521's rule lines were reverted (⛔ Jonathan's fix, gone) — both events that must be **seen**, not passed over in silence.
- The failure message states `D4` by name, that `Plugins/SiegeLlama/**` was deliberately not touched, and that the token figures are STALE.

---

## 5. ⛔⛔ A THIRD ZONE-A TEST BROKE AND NOBODY LISTED IT

**`Siegebound.Assistant.ZoneA.NullVocabularyIsNotTheMeasuredLane`** derives its expectation from the Zone-A char constant:

```
ExpectedNullLength = MeasuredZoneAChars - SynonymTable.Len() + 5
```

`NullLane` is `Snapshot->BuildZoneA(nullptr)` — **the SHIPPED builder** — so the constant is its **base**. AS-§20.4, the board, TASK-521's handoff and my dispatch all name **two** broken tests. This is a third, and it breaks for a **different** reason: not because a literal moved, but because an expectation is *computed* from the number being re-based. Re-based to the shipped figure, with the reasoning written in place (`SC-§23`: a measurement's **base** is part of the measurement; using the spike's 5116 here would compile, run, and be wrong by exactly 308).

⚠️ **The general lesson for TASK-525:** *"which tests does this edit break"* was answered by grepping for the two named tests. The right question is *"which tests read the constant"*, and it had one more answer.

`ZoneA.AsciiCleanliness` and `ZoneA.StaticPrefixContract` do **not** read the constant and are unaffected. Verified: **zero** references to the old constant name remain.

---

## 6. ⛔ WHAT I COULD **NOT** TEST, AND WHY — stated rather than faked

| thing | why it is unreachable | who owns it instead |
|---|---|---|
| **The executor's exclusion filter** (subtracts / subtracts-nobody / empty-after-exclusion refusal) | `SelectUnitsForOrder` walks a `UWorld` for `ASummonedUnit` actors. These are `EditorContext` **simple** tests: no world, no actors. My spec item (7) forbids building a world fixture, and TASK-522's own handoff says the same from the other side. | **TASK-522's three log lines** (it deliberately prints the arithmetic on the path that WORKED, because a log that only speaks on failure cannot prove a success) + **Jonathan at TASK-527**. ⛔ **Not a test.** |
| **The empty-after-exclusion refusal reaching the player** | Same reason. It routes `SelectUnitsForOrder → false → ExecuteAndReport → PushMessage(AskUnsupported)`, all of which need a live component and world. ✅ I **read** the shipped path and confirm it is a real refusal with a player-visible outcome and the arithmetic in the log — ⛔ **not a silent no-op** — but reading is not measuring. | TASK-522's log + TASK-527. |
| **`who:"none"` + exclusion → `ExcludeConflict`** | ⛔ **Structurally unreachable from JSON.** `bWhoIsNone` is set only in the parser's **String** branch; `ExcludeKinds` is filled only in its **Object** branch. No single `who` value can produce both. The shipped guard is honestly labelled *defensive* in the source, and I agree with that labelling. Test 7 asserts the **reachable** neighbour (`who:"none"` still → `who_required`, unchanged) and documents the unreachability in place. | Nothing — and nothing is owed. It is a wire-path guard for M8 P2. |
| **`NativeOnPreviewKeyDown` — the `Z` accept, and "Escape is never consumed"** | The override is `protected`; its whole gate (`bConsoleOpen` / `bConsoleEnabled` / `bConfirmPromptVisible`) is `private`; and reaching those states needs a player controller, Slate focus and a live FSM. ⛔ Driving it would require a UHT-generated test subclass and a widget tree — a different, larger task. **What I could test, I did** (Test 13): the accept-key **resolution** the handler compares against, including ⛔ that no letter position ever resolves to `EKeys::Escape` on any layout the shipped table can produce — so **Escape cannot become the absorbed key by that route**. ⚠️ **That is one half of the claim.** The other half — that the handler returns `Unhandled` for Escape — is a **code-reading** verdict for TASK-525 and a feel check for TASK-527. ✅ I read it: the token `Escape` **does not appear** in the gate at all, the fall-through goes through `Super`, and I believe the implementation is correct. | **TASK-525** (read) + **TASK-527** (feel). |
| **TASK-520's `Cancelled` transcript line** (fires from `AwaitConfirm`, silent from Idle/Composing/Thinking/Deferred/Failed) | `NotifyConsoleClosed` is reachable, but `AwaitConfirm` is entered **only** through the private `EnterAwaitConfirm()` at the end of a full model turn, and `ClearConfirmPreview` / `PushMessage` need a live owner. There is **no headless path into that state**. ✅ I read the shipped code and the state gating is exactly as specified — `bDiscardedPendingOrder` is sampled **before** the discard (correct: the discard destroys the evidence), the line is pushed **last** (correct ordering), and `Deferred` returns early so the latch survives. | **TASK-525** (read) + **TASK-527** (feel). |
| **The player-facing `Z` prompt string** | An on-screen pixel/human check (`AS-§6` ruling A(e)). ⛔ Not assertable here and not asserted; noted by `AddInfo` so nobody reads its absence as an oversight. | **TASK-527**. |

---

## 7. ⚠️ DISAGREEMENTS AND FINDINGS FROM READING THE FINISHED CODE — ⛔ NOTHING WAS FIXED; TASK-525 RULES

### D1 — ⭐ `AS-§20.3`'s "~6 chars of headroom" is the **single-digit-count** reading, and an ordinary board is already over it

**MEASURED, not inferred.** Every roster row prints its count **three times** (`total` / `orderable` / `followable`), so a board with **two-digit** counts is **3 chars wider per row = +39 across 13 kinds**. Reproducing the shipped operating point (head 108, tail 157 at the default 61-char `order:` line, budget 628):

| board | roster block | outcome at the shipped 61-char order |
|---|---|---|
| 9 per kind (single digit) | 621 | **13 rows print**, `other_kinds: none`, ~7 chars spare — this is AS-§20.3's figure |
| 12 per kind (ordinary mid-match) | 660 | ⛔ **ALREADY COLLAPSES ONE KIND — and it is the Sorcerer** |

⇒ **The risk table's number is optimistic by a whole board-state dimension.** ✅ **This does not invalidate the batch — it vindicates TASK-517's ruling 4.** The Sorcerer's **name** survives the collapse on both boards, which is precisely the *"a collapse may hide a kind's NUMBERS, never its NAME"* invariant, and it is why the fix is the `other_kinds:` names and **not** the cap.
⛔ **I deliberately did NOT assert this.** Pinning "an ordinary board collapses" would make **TASK-528's `ZoneBCharReserve` repair fail this file for succeeding**. It is reported via `AddInfo` inside Test 1 (both readings printed side by side), and Test 4 sweeps **both** magnitudes so the invariant is measured on both. **TASK-525 should decide whether `AS-§20.3` gets an amendment naming the count magnitude.**

### D2 — ⚠️ The board contradicts itself about who owns `Tests/SiegeAssistantGrammarTest.cpp`

TASK-518's board entry item **(b)** says *"TASK-523 re-bases it"* for `Grammar.SelectionCap`. My `names:` block and RULING-1's file-ownership table do **not** list that file for anyone. Two of three sources (the board entry + my dispatch) assign it to me, so **I honoured the assignment** and kept the edit minimal: **one count re-based 3 → 4** plus **one added `except` reference check**, with the reasoning written in place. ⛔ Nothing else in that file was touched — in particular I did **not** add `except`/`exceptlist` omission checks to `Grammar.DegenerateInputs`, because my own Test 12 covers them and expanding into an unowned file twice is not scope discipline. **TASK-525 rules whether the `names:` block should be amended.**

⚠️ I re-based the **count**, not the **strictness**. The obvious repair for a broken count assertion is `>= 3`, which would then pass for any future widening of `who` **including an accidental one**. `who`'s alternative set is a schema; an exact count is the only assertion that notices a fifth shape arriving unannounced.

### D3 — ⚠️ The `SiegeAssistantValidateSelection` call at `SiegeAssistantComponent.cpp` is **provably dead for the model path** — the comment above it overstates its live protection

`ParseSiegeAssistantCommand`'s **final gate** is `SiegeAssistantValidateSelection(Parsed.Kinds, Parsed.Counts, SelectionError, Parsed.ExcludeKinds)` — the **same four-argument call**. So by the time `HandleModelCompletion` runs it again, every invariant has already been checked with the same arguments on the same values. ⇒ **It cannot fire on the parse path today.** ✅ **This is not a defect and I am not asking for a change** — the header says exactly why it exists (M8 P2 receive-side, *"an invariant that is only true because of how we happen to build the value is not an invariant"*), and I agree with keeping it. ⚠️ **But it changes what a QA grep for that argument proves:** dropping it would raise **no compiler diagnostic *and* no test failure and no behavioural change today** — the loss is entirely in the future wire path. My Test 11 measures the delta the argument buys and says so in its own name and message; it is honest about not observing the call site (an `EditorContext` test cannot reach `HandleModelCompletion`). **TASK-525 should treat the call site as a code-reading item, not as something a green suite covers.**

### D4 — 📌 `AS-§20.1` spells the GBNF rule `except_list`; the emitter ships `exceptlist`

Correctly, and it is a properly **declared departure** (`SC-§15`) with the reason in the source: llama.cpp reads a rule name as `[a-zA-Z0-9-]` and **stops at the underscore**, so `except_list` would parse as the name `except` and the **whole grammar** would be rejected — the identical defect `at_least` shipped with, which cost TASK-413 two of its six bars. `AS-§20.1` itself pre-authorised `exceptlist` as the substitute. ✅ **No disagreement — recorded because a reader comparing the law to the code will hit it, and Test 12 asserts `except_list ::=` is absent so the underscore spelling can never come back.**

### D5 — 📌 TASK-518's handoff GBNF was hand-transcribed; I verified against the emitter and it holds

My dispatch flagged this. Every grammar claim in Test 12 is read off `USiegeAssistantGrammar::Build` at run time, never off the handoff. ✅ **The transcription was accurate** on everything I check. Recorded so the verification is on the record rather than assumed.

### D6 — ⚠️ `ParseKindSymbol` inside `all_except` reports `bad_type:kind`, not `bad_type:all_except`

A non-string **element** of the exclusion array is reported with the `kind` payload, because symbol validation correctly **reuses `ParseKindSymbol`** (⛔ no second kind-validation path, exactly as `AS-§20.1` demands). Only a non-array **value** for `all_except` reports `bad_type:all_except`. ✅ **Correct behaviour, and the right trade** — but the two are easy to confuse when writing an expectation, so Test 9 asserts the actual codes and this note exists so a future author does not "fix" the payload.

### D7 — ⛔ **A CAVEAT ON THE TWO-LANE TEST THAT SURVIVES MY RE-BASE AND IS NOT MINE TO CLOSE**

The original file already flagged it and it is **still true**: this test proves the **CODE-DEFAULT vocabulary** lane matches, and says nothing about the **`/Game/Data/DA_AssistantVocabulary` ASSET** lane — and the asset **overrides the defaults wholesale at runtime**. So the shipped 5424 is the figure for a session where the asset happens to match the C++ defaults. Untouched by me, restated here because `D4`'s arithmetic inherits it.

---

## 8. ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐ **The reflection fixture** (`MakeSnapshotWithRoster`). It writes `UnitKinds` / `KindTotals` / `KindOrderable` / `KindFollowable` / `PlaceNames` / `StanceFree` through `FindFProperty`, then runs the **shipped** `BuildZoneC` unmodified. **The failure mode to check is the vacuous pass:** a renamed field would make `FindFProperty` return null, leave the snapshot empty, and let a collapse assertion pass because there was nothing to collapse. **Every lookup checks presence AND the inner property type, and a miss is a named error that aborts the test.** ⇒ Verify I did not leave a path where an unusable fixture reaches an assertion.
2. ⭐ **Read every assertion's MESSAGE and ask whether the comparator can detect what the message promises** (my spec item 6). The two I would attack first: Test 2's `TestFalse(" kinds, " absent)` — does that string really only occur in the retired format? — and Test 11's name (`…IsLoadBearing`), which claims a property about a **call site** the test cannot observe; §6 and the test's own header say so explicitly, and I would rather be told the name is still too strong than have it read as a stronger guarantee than it is.
3. **Every collapse pre-condition.** Tests 2 and 4 both **assert that a collapse actually happened** before reading anything from it. If a future budget change stops the trimmer firing, these must go RED, not green-and-meaningless. Check I got that right in both.
4. **`TestEqualSensitive` everywhere an FString is compared** (`SC-§13`). ⛔ There is **no** `TestEqual` on an `FString` value in either file — verified mechanically. The `TestEqual` calls that remain compare `int32`s (counts, lengths, arities).
5. **The `D4` derivation in `SiegeAssistantZoneATest.cpp`.** ⛔ The check that matters: **the frozen spike fixture is byte-identical to what it was**, and the three `D4_*` literals are exact copies of the shipped/spike lines rather than paraphrases. A paraphrase would make the diff assertion pass against a Zone A that is not the one that shipped.
6. **`AddExpectedMessagePlain` scoping.** Five calls, each naming ONE message and never suppressing warnings wholesale — a blanket suppression would hide the two player-text truncation latches. Test 2's is `Occurrences == 0` (**must fire**) and is therefore an assertion; the other four are `-1` (ignore).
7. **Whether Test 4's sweep is too slow.** 2 magnitudes × 61 rungs = **122 `BuildZoneC` calls**, each allocating a fresh snapshot. Each build is a few hundred characters of string work with no world and no I/O, so I expect it to be immaterial — but I could not run it, so it is flagged rather than claimed.

---

## 9. STATUS + ROUTING

- **Status:** `ready-for-qa`. **QA gate named: TASK-525** (`.claude/pipeline/qa/TASK-525.md`).
- ⛔ **NOT compiled, NOT run.** TASK-526 owns the only compile and the only suite run. **Baseline for comparison: TASK-514 — `Result: Succeeded`, 0 errors / 0 warnings, suite 40/40.** This task adds **13** tests and re-bases **4** existing ones (3 in `SiegeAssistantZoneATest.cpp`, 1 in `SiegeAssistantGrammarTest.cpp`).
- ⛔ **No pinned contract was changed.** Where reading the finished code raised a question, it is in §7 for TASK-525 to rule — not fixed.
