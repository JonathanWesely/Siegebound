# TASK-476 — [FT-A1] `repeats=<N>` on `Siege.Llama.SpikeEval` — §12h's repeat law made mechanical

- **Agent:** gameplay-programmer
- **Date:** 2026-08-03
- **Files touched:** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` — **one file, nothing else.**
  ⛔ No header changed, no new file created, no `.Build.cs` touched.
- **Compile:** ⛔ **NONE RUN**, per the spec. This batch has exactly two compile gates (TASK-481, TASK-492) and each
  needs a quiet module across both live batches. Everything below that a compiler would prove is labelled
  **UNVERIFIED WITHOUT A COMPILE** in §5.
- **Git:** ⛔ **none.** No add, no commit, no branch. `main` is ahead of origin; Git is build-master's.
- **Editor / MCP / PIE:** ⛔ **none.** No model was loaded, no measurement was taken. **No number in this file is a
  measurement** — §4 is a proof about *bytes of source*, not a run.
- **Gate:** TASK-480.

---

## 1. WHAT WAS ADDED — the four spec points, each with its anchor

| Spec | Delivered | Where (by **symbol**, never by line — §18c: this file moved by hundreds of lines today) |
|---|---|---|
| **(1)** `repeats=<N>`, default 1 | `FSpikeOptions::EvalRepeats` + parsing in `ParseOptions` + the loop in `RunSplitRepeated` | new field at the **end** of `FSpikeOptions`; parse block immediately **before** `Options.ContextTokens = FMath::Clamp(...)` |
| **(2)** per-row `stable=N/N` / `flips=k` + **the identity of every row that moved** | `RunSplitRepeated`'s per-row stability block; `FSplitRowObservation` | rows recorded in `RunOneSplit`, compared in `RunSplitRepeated` |
| **(3)** split-level `min`/`median`/`max`, **labelled** | `SummariseIntSeries` + the `LENIENT` / `STRICT` / `parse_failures` lines | `SummariseIntSeries`, `FormatIntSeries` |
| **(4)** queue-depth-1 respected | **no `StartJob` call was added.** The loop is *inside* the job `CmdSpikeEval` already started | `RunEvalJob` → `RunSplitRepeated` → `RunOneSplit` ×N |

**On (3) — the labelling is the deliverable, not decoration.** §10 clause #1 scores the **MINIMUM** of the runs,
clause #6's dev regression rule scores the **MEDIAN**, and nothing scores the maximum. Two clauses, two aggregations,
one instrument ⇒ each printed number names its own aggregation, and the **per-run series** (`per_run=[19,20,20,19,20]`)
travels beside it so the aggregation is *checkable* rather than trusted. Even-N medians are the mean of the two middle
values and print with one decimal — **a silently floored 19.5 → 19 would move a gate.**

**On (2) — `stable=` vs `flips=` are defined in the output itself, because an undefined field in a gate instrument is
the defect §12h is about.** A row is `stable=N/N` when it produced **byte-identical output AND an identical verdict in
every run**; otherwise it prints `flips=k` (run-to-run **transitions**, `0..N-1`, so an A/B/A/B alternation reads higher
than a single settle) and `variants=v` (**distinct** outputs). Every unstable row then dumps **each distinct variant with
the run numbers that produced it** — which is what turns DEV-11's known `n:2` ↔ `n:all` flip from an anecdote somebody
remembered into a reading somebody can grep.

**Also added, on the same gating:** a `LENIENT` line that carries the **leniency floor** beside the distribution (§12a:
the floor travels with the number, every time); a `strict_equals_lenient_in_every_run=` line naming the runs that
disagree (§10 clause #2); and a `parse_failures` line whose **maximum** is flagged as the figure that matters
(clause #4 requires zero in *every* run).

⛔ **The block prints NO gate verdict.** §10's gate is conjunctive over **nine** clauses, five of which this command
cannot see (the artifact's sha256, the latency budget, the prompt-shape diff…). A harness printing `GATE: PASS` from the
four clauses it *can* measure would be this project's recurring defect — a confident green describing something else.
This is a **boundary, stated in the code as a comment**, not an omission.

---

## 2. ⚠️ THE ONE JUDGEMENT CALL, DECLARED — the KV cache is CHAINED across repeats and I did **not** change it

Flagging this for QA explicitly rather than letting it be found later. **`RunGeneration` reuses the token prefix shared
with the *previous* prompt**, so how much is re-prefilled on a row depends on **which row ran before it**.

- Within a split the predecessor is fixed **except for the first row**: in repeat 1 it is whatever preceded the split;
  in repeats 2..N it is the split's **own last row**.
- ⛔ **And the consequence is not confined to that one row** — row 2 reuses the cache row 1 left behind, so a different
  re-prefill boundary on row 1 **can carry down the split.** The claim that survives is the narrow one:
  **repeats 2..N are mutually identical conditions; repeat 1 is the odd one out.** The printed CAVEAT says exactly that
  and no more.

**Why I did not "fix" it by clearing the cache per repeat (§20 — the obvious repair has a consequence):**

1. Clearing before **every** repeat changes row 1's `prefill=`/`reused=` columns **at repeats=1 too** ⇒ the default path
   would no longer be byte-identical. ⛔ **§16 forbids that outright.**
2. Clearing only **between** repeats leaves the *same* asymmetry (repeat 1 differs from 2..N) — it does not remove the
   confound, it relocates it. **Both options cost exactly one boundary; only one of them also touches the most delicate
   code in the file.**
3. Choosing the replicate design is **M0's job** — a corpus-free determinism probe — not this flag's. §12h's leading
   hypothesis for the DEV-11 flip *is* KV-cache nondeterminism, and it is explicitly **not diagnosed**; a task may not
   spec a fix as though it were a finding.

⇒ **Minimum change, confound printed at WARNING level on every repeat run.** If Stage B wants identical-cache
replicates, that is a follow-up with a stated reason. **QA should scrutinise this decision specifically.**

---

## 3. WHAT I DELIBERATELY DID **NOT** ADD (and why) — so it doesn't read as an omission

- ⛔ **No warning when `holdout=` is passed without `repeats=5`.** It would be genuinely useful, and I refused it: it
  fires on a command line that **exists today and passes none of my new arguments**, so it would change the default
  output of the holdout path. §16 freeze #2 is unconditional. **Named here so the gap is a decision, not an oversight.**
- ⛔ **No `repeats=` warning inside `CmdSpikePrompt`.** `SpikePrompt` generates nothing, and **TASK-477 owns that
  function** — I kept my hands off its collision surface. The warning for job kinds that ignore `repeats=` lives in
  `StartJob` instead, which covers Load and Bench, the two that actually generate.
- ⛔ **`t0`'s bytes, `AppendZoneA/B/C`, `BuildSpikeGrammar`, `VerifyFixtureKindParity`, `BuildPrompt`, `RunGeneration`,
  `ScoreRow`, `LoadCorpus`, `CmdSpikePrompt`: NOT TOUCHED.** Confirmed by the diff's removed-line set (§4).

---

## 4. 🔒 PROOF THE DEFAULT PATH IS UNCHANGED — mechanical, not eyeballed

**The mechanism:** every pre-existing eval log line gained exactly **one `%s`**, spliced in directly after `split=%s`,
fed by `*RepeatTag`. `RepeatTag` is **the empty string whenever `Repeats == 1`** (it is only ever assigned inside
`if (Repeats > 1)`). Rendering `%s` with `""` contributes **zero bytes**. Everything genuinely new is emitted from a
block `RunSplitRepeated` **returns before** at `repeats <= 1`.

**(a) Every pre-existing line survives and reduces exactly to its old form.** Script: take each *new* format string
containing `split=`, replace `split=%s%s` → `split=%s` **once**, and require character-identity with a *HEAD* format
string; then require the leftover pool of old strings to be **empty** (a removal would also be an output change).

```
OLD format strings containing split= : 8
NEW format strings containing split= : 22
A. reduce exactly to an old string  : 8 matched   (all 8, "TAG INSERTED")
B. old strings with no counterpart  : (none)      <-- nothing was dropped
C. genuinely new strings            : 14
D. every new string on the TagEvalRepeat/TagWarn stream : 14/14 OK
```

**(b) The one changed line without a `split=` token, checked by hand — character-identical, moved not edited:**

```
HEAD  3896:  UE_LOG(LogSiegeLlama, Error, TEXT("%s: split '%s' NOT SCORED -- %s"), TagEvalScore, *SplitName, *LoadError);
NEW   4289:  UE_LOG(LogSiegeLlama, Error, TEXT("%s: split '%s' NOT SCORED -- %s"), TagEvalScore, *SplitName, *LoadError);
```

It was `RunOneSplit`'s **first statement** with nothing before it, and now it is `RunSplitRepeated`'s first statement,
called from where `RunOneSplit` was called ⇒ **same text, same `Error` level, same tag, same position in the stream.**

**(c) Nothing behaviour-bearing was added inside `RunOneSplit`.** The complete set of additions to its body is
`OutTotals.Observations.Reset()/Reserve()`, five `Observation.*` assignments, five `OutTotals.*` assignments, and the
`*RepeatTag` arguments. **No `return`/`continue`/`break`/branch was added to that function** (verified over the diff);
the new control flow lives only in the new helpers and in `RunSplitRepeated`.

**(d) Structural + format checks, each with a positive control (§14):**

| Check | Result |
|---|---|
| Brace / paren balance (strings & comments excluded) | `depth=0`, `paren=0`, `min_depth=0`, no unterminated literal — **BALANCED** |
| `UE_LOG` specifier-vs-arg count, **all 103 calls in the file** | **0 mismatches**; 20 are TASK-476 lines, **83 untouched lines are the positive control** — they balance, so the checker works |
| Non-ASCII inside `TEXT()` literals | **0.** The file has **no BOM** and every pre-existing log string is ASCII; I found one `⇒` I had introduced and replaced it with `=>`. Emoji stay in comments, matching the file's convention. |
| `git grep` positive control before trusting "the name is free" | `RunOneSplit` → 3 hits, file **is** tracked ⇒ the negative result for `repeats`/`EvalRepeats`/`SPIKE_EVAL_REPEAT` is trustworthy |

**⚠️ WHAT (a)–(d) DO AND DO NOT ESTABLISH.** They prove the **source bytes** can only render the old lines at
`repeats=1`. They are **not** a run. ⇒ **TASK-481 still owes the INSTRUMENT-UNCHANGED proof by re-running the existing
dev-split measurement and showing the number unmoved** — §16 requires that, and nothing here substitutes for it.

---

## 5. ⛔ UNVERIFIED WITHOUT A COMPILE — stated plainly

Nothing below is a known defect; each is a claim only a compiler settles, and I was ordered not to run one.

1. **Overload/type resolution** on the new code: `TArray<int32>::Sort()`, `FString::FromInt`, `AddDefaulted_GetRef()`,
   `TArray::SetNum` on a struct holding a `TArray`. All are standard UE5 surface and are used elsewhere in the module,
   but none is compiler-confirmed **in this file**.
2. **`ESearchCase::CaseSensitive` on `FString::Equals`** — it is the documented default and is passed explicitly per
   §13; not compiler-confirmed here.
3. **Declaration order.** `FSplitRowObservation`, `FSplitRunTotals`, `SummariseIntSeries`, `FormatIntSeries` are
   declared **above** `RunOneSplit`; `RunSplitRepeated` sits **between** `RunOneSplit` and `RunEvalJob`. Verified by
   reading, not by a build.
4. **No unused-symbol warnings.** `FSplitRunTotals::QuestionPasses` is written and never read (kept for symmetry with
   the per-run line and for TASK-477/478's use); it is a struct field, not a local, so it should not warn — **unproven.**
5. **`%.1f` with a `double`** in `UE_LOG` — arg counts are checked (§4d) but not the *types*.

---

## 6. ⚠️ EXACTLY WHICH SYMBOLS I TOUCHED — for TASK-477 → TASK-478 to bound the collision

**Same file, strictly serial (manager ruling 11). Nothing below was left half-done.**

**NEW symbols (all free names — confirmed against a positive control, §14):**
`SpikeMaxEvalRepeats` · `TagEvalRepeat` (= `"SPIKE_EVAL_REPEAT"`) · `FSpikeOptions::EvalRepeats` ·
`FSplitRowObservation` · `FSplitRunTotals` · `SummariseIntSeries()` · `FormatIntSeries()` · `RunSplitRepeated()`

**MODIFIED symbols:**

| Symbol | What changed | Collision note for 477/478 |
|---|---|---|
| `RunOneSplit()` | **SIGNATURE CHANGED** — now `(Options, ZoneAText, SplitName, const TArray<FCorpusRow>& Rows, const FString& RepeatTag, FSplitRunTotals& OutTotals)`. **No longer takes `CorpusPath` and no longer loads the corpus.** | ⚠️ **The biggest change in the file.** Anything calling it must use the new shape. |
| `RunEvalJob()` | both call sites now call `RunSplitRepeated` | low |
| `ParseOptions()` | one **contiguous** insertion immediately **before** `Options.ContextTokens = FMath::Clamp(...)` | ⚠️ **TASK-477 adds `out=`/`ids=` here too — same function, adjacent region.** |
| `FSpikeOptions` | one field appended at the **end** of the struct | ⚠️ **TASK-477 adds fields here too — append after mine.** |
| `StartJob()` | one warning block inserted **before** the `World == nullptr || !World->IsGameWorld()` check | low |
| `GSiegeLlamaSpikeEvalCommand` | help string extended to document `repeats=` | low — but see the flag below |

**UNTOUCHED and safe for 477/478:** `CmdSpikePrompt` · `BuildPrompt` · `TokenizePrompt` · `BuildSpikeGrammar` ·
`VerifyFixtureKindParity` · `SpikeFixtureT0` / `SpikeFixtureT1` · `AppendZoneA/B/C` · `SpikeRoster` · `RunGeneration` ·
`ScoreRow` · `LoadCorpus` · `FCorpusRow` · `CmdSpikeGrammar`.

### 📌 A FINDING TASK-477 NEEDS BEFORE IT STARTS — `chat=0|1` **ALREADY EXISTS AND IS ALREADY WIRED**

TASK-477's spec says to add `chat=0|1` **on both commands**. Checked against the mechanism, not the spec:

- `FSpikeOptions::bUseChatTemplate` exists and **defaults to `true`**.
- `ParseOptions` already contains `GetArgBool(Args, TEXT("chat"), Options.bUseChatTemplate);` — and because
  **`ParseOptions` is shared by every command**, `chat=` is already parsed for `SpikeEval` *and* `SpikePrompt` today.
- `BuildPrompt` already honours it: `false` ⇒ raw concatenation; `true` ⇒ `llama_chat_apply_template` with
  `system` = Zone A, `user` = Zone B+C, `add_generation_prompt = true`, **with a documented fallback to raw concat if
  the template call fails.**

⇒ **TASK-477 should verify rather than re-add**, or it will ship a duplicate flag. ⚖️ **This is a §15 call for
TASK-477 to make, not one I may make on its behalf** — I am reporting the mechanism, not refusing its spec line.
✅ **Consequence for Stage B: M1 (`chat=1` ×5 then `chat=0` ×5) is now exactly two commands** —
`SpikeEval chat=1 repeats=5` and `SpikeEval chat=0 repeats=5` — which is the payoff this task was ordered first for.

---

## 7. WHAT QA SHOULD SCRUTINISE (ranked)

1. **§2 — the chained-KV decision.** The judgement call of this task. Is "minimum change + printed caveat" right, or
   should repeats be forced into identical cache conditions? I argue the latter is M0's decision.
2. **§4 — the byte-identity proof.** The claim is *"only an empty `%s` was inserted"*. The scripts are in the scratchpad
   and re-runnable; **please re-derive rather than accept** (the RELAYED-DIAGNOSIS LAW applied to my own proof).
3. **The help-text change on `GSiegeLlamaSpikeEvalCommand`** — the one string I changed **that is not gated on
   `repeats>1`.** I judged the console *registration blurb* to be outside §16's frozen *measurement output* contract, and
   an undocumented flag to be a worse defect. **If QA rules the other way, it reverts on its own.**
4. **`flips=` semantics** (adjacent transitions, `0..N-1`) vs `variants=` (distinct outputs) — both are defined in the
   log line itself. Confirm the definitions are the ones §12h wants.
5. **The `bVerdictMovedWithoutBytes` ERROR path** — fires only if identical bytes ever score differently, i.e. if
   `ScoreRow` carries state. ⚠️ **Per §32 this mechanism has never been observed to function** and cannot be, without a
   run; it is an assertion, not a tested safeguard.
6. **Median on even N** = mean of the two middle values, printed `%.1f`. Confirm that is the intended reading for
   §10 clause #6.

---

## 8. STATUS

Board set to **`ready-for-qa`** (gate **TASK-480**). ⛔ Not marked passing by me.
⚠️ **TASK-477 must not start until this is gated** — same file, strict order 476 → 477 → 478.
