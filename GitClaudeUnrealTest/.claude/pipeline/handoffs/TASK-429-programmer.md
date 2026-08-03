# TASK-429 — THE HARNESS TELLS THE TRUTH: the `deadline=` knob, the ACTUAL thread count, the WARN-R2 fix (gameplay-programmer)

**Status: ready-for-qa. NOT compiled, no Git, no editor, no board edit** (the dispatch forbade all four; the
orchestrator must flip the board status, not me).

**M8 DECLARATION (verbatim):** adds no replicated property, no new replicated class, no new relevancy tier.

---

## 0. THE ONE-PARAGRAPH VERSION

Three changes, all in `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp`, all instrumentation.
**(1)** `deadline=<seconds>` overrides the hard abort ceiling — **default unchanged at 10.0** — and every line that
prints it also prints **which regime it is in**, because `abort_callback` is header-documented CPU-only.
**(2)** The **actual** `n_threads` / `n_threads_batch` are now queried and printed beside the requested values, with
the machine's physical core count beside them. ⚠️ **I could NOT settle the question statically, and the reason is
itself a finding: the requested value is 14 (from four prior runs), the actual has never been observable in this
project by any means, and the plugin installs no llama log callback so llama.cpp's own `n_threads` line has never
reached a log.** **(3)** WARN-R2 is fixed: the layout verdict is gated on a bound that completed **both** turns, an
incomplete bound reports the `llama_decode` code and where it stopped instead, and the shout-for-a-real-defect
direction is preserved and traced below. ⚠️ **I also changed one thing beyond the three — declared in §6.**

---

## 1. ⚠️ WHAT I DID NOT TOUCH — and the mechanical proof, not the assertion

⛔ Not touched: **Zone A** (`AppendZoneA`) · the **Zone A mirror** in `SiegeAssistantSnapshot.{h,cpp}` (never
opened) · the **GBNF** (`BuildSpikeGrammar`, and `SiegeAssistantGrammar.{h,cpp}` never opened — in flight) · the
**t0/t1 fixtures** · **`ScoreRow`** / any scoring rule · **`BuildPrompt`** / `AppendZoneB` / `AppendZoneC` · **any
corpus CSV** · `MaxSnapshotChars` / `ZoneBCharReserve` / `MaxRosterKinds` / `MaxUtteranceChars` / `ContextTokens` /
`SiegeAssistantMaxSelectionKinds` · **`SpikeHardTimeoutSeconds`'s value** (still `10.0`) · CONVENTIONS §10 · the
board.

⛔ **I did not open `assistant_eval_holdout.csv` or `assistant_eval_holdout2.csv`.** I did not read TASK-427's
handoff or output. The only corpus path I touched in code is the **log line** for a row, never a row's content.

**The proof QA can re-run in one command** (`git diff -U0` hunk headers on the file, which currently carries
TASK-428's changes *and* mine):

```
@@ -222 +222 @@      static void AppendZoneA   <- TASK-428
@@ -276,0 +277,8 @@   static void AppendZoneA   <- TASK-428
@@ -297 +305 @@       ... six more AppendZoneA hunks, all TASK-428's
@@ -327,0 +344,8 @@   static void AppendZoneA   <- the LAST TASK-428 hunk
--------------------------------------------------------------- every hunk below is MINE
@@ -2175,0 +2200,26 @@  struct FSpikeOptions
... (30 hunks) ...
@@ -4320 +4749,2 @@   GSiegeLlamaSpikeEvalCommand
```

**Every hunk of mine is at old-file line ≥ 2175.** `AppendZoneA` spans lines **220–354** and ends 1800+ lines above
my first edit; the fixtures are at **517–571**; the GBNF at **858+**; `ScoreRow` at **1816**. None is inside any
hunk of mine. **Bar #3's 77.1 % and bar #4's footprint are reproducible from this tree**, and TASK-428's
byte-identical mirror is untouched — I did not need to re-verify it because I never entered the function.

Corroborating: brace balance is **-3 at HEAD and -3 now** (both from `{` inside string literals), i.e. unchanged.
No non-ASCII byte appears inside **any** string literal in the file (checked mechanically across the whole file, not
just my lines) — the ⚠️/⛔ markers are comments-only, matching the file's existing practice.

---

## 2. ⚠️ THE THREAD LEAD — what I can and cannot tell you without running it

### 2a. What the code did (the defect, confirmed)

`SiegeLlamaSpike.cpp` printed `threads=%d` from **`ContextParams.n_threads`** — the value handed *in* to
`llama_init_from_model`. `n_ctx`, `n_batch` and `n_ubatch` were all corrected to read the actuals under
`qa/TASK-412.md` WARN-2; **threads was the fourth member of the same struct and was missed by that pass.** The
vendored header is explicit (`llama.h:551-556`, anchor **verified exact**):

> *"After creating a llama_context, it is recommended to query the actual values using these functions … the
> requested values via llama_context_params may differ from the actual values used by the context"*

### 2b. ⚠️ THE STATIC ANSWER: the REQUEST is 14, the ACTUAL is genuinely unknown, and that second fact is a finding

Four prior runs are on disk in `Saved/Logs/`, and all four print the same figure:

```
SPIKE_LOAD tier=cpu       gpulayers=0/36  ctx=2048(req 2048) batch=512 ubatch=64(req 64) threads=14
SPIKE_LOAD tier=partial   gpulayers=18/36 ...                                            threads=14
SPIKE_LOAD tier=full      gpulayers=-1/36 ...                                            threads=14
SPIKE_LOAD tier=gpulayers18 ...                                                          threads=14
```

⇒ **REQUESTED = 14**, and since none of those commands passed `threads=`, 14 is `DefaultThreadCount()` =
`max(1, FPlatformMisc::NumberOfCores() - 2)`, so **this machine reports 16 physical cores.** That inference is
stated as an inference; the new `cores=` field prints it directly from now on and QA should prefer that.

⛔ **The ACTUAL cannot be determined from anything in this repository, and I want to be exact about why rather than
hedging:**

1. The vendored artifact is **binaries only** (`llama.dll` + import lib, release `b10235`); there is **no
   llama.cpp source tree** here to read the constructor's clamping logic out of.
2. The plugin **installs no `llama_log_set` / `ggml_log_set` callback** — I grepped every `Private/*.cpp` and there
   is none. llama.cpp's own `llama_context: n_threads = N / n_threads_batch = N` line therefore **has never reached
   a UE log in this project**, and does not appear in any of the saved logs (checked).
3. So the actual thread count has been **unobservable by any available means** until this line exists. **That is the
   whole point of the task, and it also means I must not guess at the answer** — the instrument is built; TASK-431
   reads it.

**What the new line will settle in one reading:** `threads=A(req 14) threads_batch=B(req 14) cores=16`. If **A ==
14** the request was honoured and the CPU tier's latency is a real decode-rate figure. If **A is 1, or 4, or
anything well below 14**, that is a **root cause and a bug** — a CPU tier running on a fraction of the machine —
and the benchmark matrix in TASK-431 (b)/(d) is measuring a misconfiguration, not hardware. A dedicated
`SPIKE_WARN` fires automatically on any mismatch, naming both figures and the core count.

### 2c. ⚠️ A related observation I did NOT act on — flagged, not fixed

`ContextParams.n_threads_batch = ContextParams.n_threads;` sets **prefill** threads equal to **generation** threads
(both `cores - 2`). That is a deliberate choice with a stated reason (`DefaultThreadCount`: *"leave two cores for the
game"*), not a bug — but it means **`threads=N` moves both at once and there is no way to vary them
independently.** If TASK-431's diagnosis wants to separate the CPU tier's **TTFT** cost (~2716 ms, prefill-bound,
governed by `n_threads_batch`) from its **decode** cost (156-182 ms/token, governed by `n_threads`), that lever does
not exist. Adding a `threads_batch=` arg was **not specced and I did not add it.** Naming it so the diagnosis is not
silently limited by an absence.

### 2d. THE OTHER REQUESTED-VS-ACTUAL SWEEP — one more found, one gap closed, one non-issue recorded

The dispatch asked me to fix any other value still printed as fact. I audited **every** use of `Options.*` and every
field of `ContextParams`:

| field | before | after | verdict |
|---|---|---|---|
| `n_ctx` | `ctx=%d(req %d)` actual-first | unchanged | already correct |
| `n_ubatch` | `ubatch=%d(req %d)` | unchanged | already correct |
| `n_batch` | `batch=%d` — **actual, but the request was never printed** | `batch=%d(req %d)` | **gap closed.** The value was already the actual, so this is not a truth fix; it makes all four read in one idiom so a reader does not have to remember which of them is which |
| `n_threads` | `threads=%d` = **THE REQUEST, printed as fact** | `threads=%d(req %d)` | **THE DEFECT. Fixed.** |
| `n_threads_batch` | **not printed at all** | `threads_batch=%d(req %d)` | added — separate number, separate diagnosis |
| `n_gpu_layers` | `gpulayers=%d/%d` — **the request, printed as fact** | `gpulayers=%d(req)/%d` | ⚠️ **A THIRD INSTANCE, AND IT IS NOT FULLY FIXABLE.** `n_gpu_layers` is an **input** field (`llama.h:313`, anchor verified) and the vendored header **ships no getter** for how many layers were actually placed. There is nothing to query, so I labelled it rather than inventing a number. **Do not read `gpulayers=-1(req)` as "36 layers landed"** — `-1` is the request *"all of them"*. Recording it explicitly so a future pass does not spend time hunting for a getter that does not exist |
| `MaxOutputTokens` | a harness budget, never handed to llama | unchanged | not in this family |

⚠️ **One thing I noticed and deliberately did NOT change** (a behaviour change, not instrumentation, and out of
scope): the prompt-budget pre-check uses `llama_n_ctx`, while `llama_n_ctx_seq` (`llama.h:555`) also exists and is
the **per-sequence** context. They are equal at the default `n_seq_max = 1` and the spike only ever uses sequence 0,
so nothing is wrong today. Flagging it for **TASK-423**, which inherits this idiom into the shipped subsystem: if
that subsystem ever raises `n_seq_max`, the budget must move to `llama_n_ctx_seq` or it silently over-admits.

---

## 3. THE `deadline=` KNOB

### 3a. What it does

`deadline=<seconds>` on **`Siege.Llama.SpikeBench`** and **`Siege.Llama.SpikeEval`** (it parses in the shared
`ParseOptions`, so `SpikeLoad` accepts it too and it simply has nothing to bound). It overrides
`FSpikeOptions::HardTimeoutSeconds`, which **defaults to `SpikeHardTimeoutSeconds` = 10.0** — CONVENTIONS §10's
shipped `HardTimeoutSeconds`, **whose value I did not change**. A command line without `deadline=` measures exactly
what shipped.

`RunGeneration` now arms `GRunner.AbortDeadlineSeconds = StartSeconds + Options.HardTimeoutSeconds` instead of the
constant, and the hard-timeout warning prints the effective value instead of the constant (it previously printed
`10.0` unconditionally — which would have become an outright lie the moment the knob existed).

**Rejected values fall back loudly and are NOT clamped.** `deadline=abc` parses to `0.0` through `FCString::Atod`;
a silently clamped `0.0` would abort every generation instantly and read as a catastrophic model failure caused by
a typo. Outside `0.1 .. 600.0` ⇒ a `SPIKE_WARN` and the default is used. **Nothing is silently moved**, because a
quietly clamped ceiling produces timings under a bound the command line never asked for.

### 3b. ⚠️ HOW IT SAYS WHICH REGIME IT IS IN — the load-bearing half

`FormatDeadlineRegime()` builds every mention of the deadline, and it always states **two** things: whether the
value was **overridden**, and whether it is **authoritative or advisory**. The second comes straight from the
vendored header (`llama.h:382-385`, anchor **verified exact**): *"Abort callback / if it returns true, execution of
llama_decode() will be aborted / **currently works only with CPU execution**"*, which CONVENTIONS §8 already records
as *"a promise on CPU, a request on GPU"*.

```
deadline_s=10.0(default = CONVENTIONS section 10 HardTimeoutSeconds, AUTHORITATIVE: gpulayers=0, so
abort_callback aborts llama_decode mid-graph per llama.h:382-385)

deadline_s=20.0(OVERRIDDEN by deadline= -- NOT the shipped 10.0s ceiling, do NOT compare this run's wall times
against a default-deadline run without saying so, ADVISORY ONLY: layers are on the device and abort_callback is
documented CPU-only, so this can fire only BETWEEN graph submissions -- tokens= is what actually bounds this run)
```

The regime test is `Options.GpuLayers == 0`, which is exactly what `tier=cpu` (and `gpulayers=0`) sets.

**Where it appears:** the `SPIKE_RUN START` line of **every** job · a dedicated `SPIKE_WARN` whenever the value is
non-default · the bench `SPIKE_LATENCY ... SUMMARY` line (where the wall times are) · the hard-timeout warning ·
the WARN-R2 suppression warning. ⚠️ **A non-default deadline can therefore not be missed** — it is on the first
line of the run *and* in a Warning-level line of its own, because an unflagged non-default ceiling is a measurement
that lies.

**Not in the reload key, deliberately.** The deadline is armed per generation and touches nothing the context is
built from, so `deadline=10` → `deadline=20` does **not** reload 2.5 GB of weights between cells. `Threads` **is**
in the key (it goes into `llama_context_params`), so a `threads=` cell correctly forces a fresh load — which is
also where the new thread readback prints.

---

## 4. ⛔ WARN-R2 — THE FIX, AND THE TRACE IN BOTH DIRECTIONS

### 4a. The mechanism, traced end to end

This is the chain that produced `DROP=0.9%` + *"THE PROMPT LAYOUT IS WRONG…"* twice:

```
turn 1 stops early inside PREFILL
  -> RunGeneration's prefill-failure path returns early and does GRunner.LastPromptTokens.Reset()
     (correct on its own: the cache is in an unknown state and the next turn must not claim a prefix)
  -> turn 2 compares against a ZERO-LENGTH previous prompt  =>  CommonPrefix = 0
  -> turn 2's PrefillTokens = its whole prompt
  -> DropPercent = 100*(1 - turn2_prefill/turn1_prefill) ~ 0  (the 0.9 % is just the two prompts'
     length difference, nothing more)
  -> DropPercent < 60  =>  the layout verdict fires, at Warning volume, naming the PROMPT
```

⚠️ **The `reused=0` in the recorded line (`bound=BEST_CASE reused=0 DROP=0.9%`, CONVENTIONS §12e) is the
fingerprint of exactly this path** — a turn-1 **prefill** stop, not a decode stop, because only the prefill path
resets `LastPromptTokens`. The verdict was never about the layout; it was about a generation that did not run.

### 4b. What changed

`RunPrefillBound` now returns `bool` (= completed both turns) and reads both turns' `CompletedNormally()`
(`!bAborted && !bDecodeFailed`). Each of the **five** stop sites in `RunGeneration` now records a preformatted
`FailureDetail` naming **the `llama_decode` return code and the offset/token** — the caller could previously see
nothing at all:

| stop site | what it now records |
|---|---|
| tokenize | `TOKENIZE failed -- llama_decode was never called…` |
| prompt budget | `PROMPT_BUDGET %d prompt + %d output tokens exceed the ACTUAL n_ctx=%d…` |
| prefill decode | `PREFILL llama_decode=%d at prompt offset %d (chunk %d, n_batch %d)` + *"code 2 is ABORTED, i.e. this run hit the deadline INSIDE the prefill"* when the code is 2 |
| decode loop | `DECODE llama_decode=%d at output token %d of a %d-token budget` + the same code-2 note (*"the JSON is TRUNCATED, not wrong"*) |
| deadline break | `DEADLINE %.1fs elapsed after %d output token(s)…` |

`2 = aborted` is confirmed against the header's own enumeration (`llama.h:963-977`, anchor **verified exact**:
`0 success / 1 no KV slot / 2 aborted / -1 invalid batch / <-1 fatal`), so the pre-existing
`bAborted = (DecodeResult == 2)` is correct and I left it.

### 4c. ⚠️ THE TRACE IN BOTH DIRECTIONS — the thing QA is told to check

**DIRECTION 1 — a decode/prefill abort must be SILENCED as a layout finding.**
`bBoundCompleted == false` ⇒ the function logs a `SPIKE_WARN` naming both turns' `FailureDetail` plus the deadline
regime, and **returns before** the Zone-C-tail guard and **before** the `DropPercent < 60` verdict. The string
*"THE PROMPT LAYOUT IS WRONG"* is **unreachable** on that path. The `SPIKE_PREFILL` line still prints the drop
number (withholding a datum is its own kind of lie) but strips the Expectation text and labels it — and the two
incomplete cases are labelled **differently**, because they are not the same thing:
- **turn 1 failed** ⇒ `DROP=x% [NOT A MEASUREMENT -- turn 1 did not complete (<detail>), so turn 2 re-prefilled from
  a cleared cache basis…]`. The number is an artifact.
- **turn 1 fine, turn 2 failed** ⇒ `DROP=x% [NO VERDICT -- the drop itself is measured, but turn 2 did not complete
  (<detail>)…]`. ⚠️ **The drop here is genuinely valid** (`PrefillTokens` and `PrefixReused` are both fixed before
  the first decode), so suppressing the verdict is **conservative on purpose** — the spec is categorical that the
  verdict needs *both* turns, and I followed it rather than inventing a third rule. The wording says which case it
  is so a reader is not misled in the opposite direction.

The line also gains `status=COMPLETED_BOTH_TURNS|INCOMPLETE` so the state is greppable, and the
`BAR#3 BOUNDS` summary line — which reprints both bounds' figures in one place and is **the line a report is most
likely to be copied from** — names any bound that did not complete. Suppressing inside the function alone would
have been undone one line later.

**DIRECTION 2 — a genuine layout defect must still SHOUT.**
`bBoundCompleted == true` ⇒ execution falls through **both** guards exactly as before. A bound where both turns ran
to a natural stop and reuse was genuinely poor still fires the full-volume warning; the gate is **completion, never
the drop value**, so no reachable drop percentage is newly silenced. The message now adds *"and BOTH TURNS
COMPLETED -- so this is a real reuse figure, not an abort artifact"*, which is the sentence that makes the line
worth quoting again. **The Zone-C-tail over-reuse guard (the implausibly-HIGH direction) is likewise unchanged for
a completed bound.**

⚠️ **The only behaviour I removed is on the incomplete path, and on that path the verdict was wrong 2 times out of
2.** A completed bound's output is byte-comparable to before apart from the added `status=` field and the added
clause in the warning.

---

## 5. THE EXACT COMMAND LINES FOR TASK-431's CPU MATRIX

Pre-flight is unchanged (§12e: re-append the `Engine.ini` remote-exec block, verify USER-GLOBAL
`bThrottleCPUWhenNotForeground=False`, verify **measured FPS ≥ 58 live**, PIE on `L_Arena` with units on the field
via `SummonTestUnit`). ⛔ **No `holdout=` on anything in this task.**

**Cheapest first — the thread readback needs no bench at all:**
```
Siege.Llama.SpikeLoad tier=cpu gpu=0
```
⇒ read `SPIKE_LOAD … threads=A(req 14) threads_batch=B(req 14) cores=16 …`. **This one line answers §(c)'s
"report REQUESTED and ACTUAL".** If a `SPIKE_WARN: THE CONTEXT CLAMPED THE THREAD REQUEST` accompanies it, stop and
report — that is the root cause and the rest of the matrix is measuring a misconfiguration.

**(a) CONTROL — do the 2-of-5 aborts reproduce?**
```
Siege.Llama.SpikeBench tier=cpu gpu=0 iters=5 deadline=10
```
`deadline=10` is the default, so the log will correctly read `default = CONVENTIONS section 10 HardTimeoutSeconds`
— passing it explicitly is documentation, not an override. Read `aborted_or_failed=N/5` off the SUMMARY line
directly; it no longer has to be counted by eye. **A non-reproducing defect is a finding, not a pass.**

**(b) DEADLINE SWEEP — is the ceiling the binding constraint?**
```
Siege.Llama.SpikeBench tier=cpu gpu=0 iters=5 deadline=20
```
(escalate to `deadline=40` only if 20 does not clear). ⚠️ `gpu=0` is kept identical to (a) **on purpose**: `gpu=`
and `threads=` are in the reload key and `deadline=` is not, so (a)→(b) does **not** reload the model and the only
variable between the two cells is the ceiling. ⇒ **all 5 complete with parseable JSON ⇒ the aborts are a TUNABLE.**
The SUMMARY's `WORST_wall_ms` is then the worst-case wall to quote in the recommendation.

**(c) THREADS — explicitly at the physical core count:**
```
Siege.Llama.SpikeBench tier=cpu gpu=0 iters=5 deadline=20 threads=16
```
Use the `cores=` figure the load line prints rather than the 16 inferred here. **This cell forces a reload** (the
thread count is in the reload key), so it emits a fresh `SPIKE_LOAD` line — read the actual off *that* line, not the
earlier one.

**(d) ms/token** is already per-iteration on every `SPIKE_LATENCY` line; every aborted iteration now carries
` ABORTED(DECODE llama_decode=2 at output token N of a 96-token budget -- …)` inline, so which token each stop
landed on is readable without a second run.

⚠️ **Each cell twice, report the second** (board's instruction). ⚠️ **`wall60_equiv_ms` is an EXTRAPOLATION and must
not be quoted as a measurement** — unchanged, still printed, still labelled in the source.

⚠️ **If any cell is run on a GPU tier, `deadline=` is ADVISORY there and the log says so in the same breath as the
number** — do not compare a `tier=full` wall time against a cpu one as evidence about this knob.

---

## 6. ⚠️ ONE CHANGE BEYOND THE THREE — declared for QA to rule on, not slipped in

**The eval row line now names a generation that was CUT.** `RunOneSplit`'s per-row detail line gains
` ROW_DID_NOT_COMPLETE(<detail> -- this row scores as a WRONG ANSWER but is a STOPPED generation, not a
comprehension failure; do not tune against it)`.

**Why I judged it in scope:** this is **WARN-R2's exact defect in the accuracy lane.** A generation the ceiling cuts
emits truncated JSON, fails to parse, scores as a wrong answer, and was — until this line — **indistinguishable in
the log from a model that understood the sentence and answered it badly.** TASK-431 §(3) is a two-halved gate whose
failure route is *"route back to the programmer, this counts as a ladder loop"* — so an aborted row could burn a
ladder loop re-tuning a prompt to fix what is actually a decode-rate problem. Given ruling 10 exists precisely
because a diagnostic accused the wrong component twice, leaving the accuracy lane with the same hole seemed worse
than declaring an extra `%s`.

**Why it is safe:** it appends one field to a Display line, changes **no** score, no scoring rule, no corpus, and no
control flow. It is empty on every completed row, so a clean run's output is unchanged. **If QA rules it out of
scope, deleting the `StopSuffix` local and its `%s` reverts it with no other consequence.**

Two smaller changes in the same spirit, inside the specced surface: the bench SUMMARY gained
`aborted_or_failed=N/M` plus a `SPIKE_WARN` when N > 0 (the "2 of 5 aborted" fact was previously only recoverable
by counting ` ABORTED` suffixes by eye — and it is the single most consequential number the CPU tier produced), and
the per-iteration ` ABORTED` suffix now carries the reason.

---

## 7. FILES TOUCHED

| file | what |
|---|---|
| `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSpike.cpp` | **the only file changed.** `FSpikeOptions::HardTimeoutSeconds` + new `FormatDeadlineRegime()` · `FGenerationResult::FailureDetail` / `CompletedNormally()` · 5 stop sites in `RunGeneration` · deadline armed from options · thread actuals + `cores=` + the clamp warning in `EnsureModelLoaded` · `RunPrefillBound` → `bool` + the WARN-R2 gate · `RunBenchJob` bounds-validity, abort count, SUMMARY regime · `RunOneSplit` row stop-suffix · `StartJob` announcement + non-default warning · `GetArgDouble` + `deadline=` parsing · three console-command help strings |

**Assets referenced:** none. **No `Content/`, no Blueprint, no `.uasset`, no MCP call.**

---

## 8. ⚠️ WHAT QA SHOULD SCRUTINISE HARDEST

1. **§4c direction 2** — that a *completed* bound with poor reuse still shouts. Read the fall-through, not my
   summary: the `return false` sits above **both** guards and nothing else was reordered.
2. **The turn-2-abort case in §4c** — I suppress a verdict on a drop figure that is arguably valid. That is a
   judgement call I made against the spec's categorical wording. **If you disagree, it is a two-line change.**
3. **The threads readback is a real query** — `llama_n_threads(GRunner.Context)` / `llama_n_threads_batch(...)`,
   not a re-print. Both symbols are **exported by the vendored import lib** (verified: `llama.lib` carries
   `llama_n_threads` and `llama_n_threads_batch` at the same occurrence count as the already-linked
   `llama_n_ubatch`), so this links.
4. **The `deadline=` default is still 10.0** and `SpikeHardTimeoutSeconds` is untouched at line 142.
5. **§6's scope call** — rule it in or out explicitly.
6. **Format specifiers.** I hand-counted every changed/new `UE_LOG` and `FString::Printf` (arg count == specifier
   count on all of them, `%.1f`/`%.3f`/`%.0f` for doubles, `%d` for int32 with explicit casts off `int32_t`). The
   biggest is the `SPIKE_LOAD` line at **20 specifiers / 20 args** — worth re-counting independently.
7. **Every line-number anchor in my new comments was opened and checked**, not carried from a doc: `llama.h:313`
   (`n_gpu_layers`), `382-385` (abort_callback), `551-556` (query the actuals), `963-977` (decode return codes),
   `985-989` (the thread getters). ⚠️ **All five are vendored-header anchors, which the TASK-433 evidence says are
   the reliable kind** — I did not add any game-lane line anchor.
