# TASK-457 — gameplay-programmer handoff (2026-08-03)

**[LLM-WARN] The hard timeout is now a real backstop on every tier + WARN-1 + WARN-3.**
Plugin lane only. Two files touched, both in `Plugins/SiegeLlama/`.

**M8 DECLARATION DUTY, verbatim:** *adds no replicated property, no new replicated class, no new
relevancy tier.*

---

## ⛔ READ THIS FIRST — FIVE THINGS THIS HANDOFF WILL NOT LET YOU BELIEVE

1. **`HardTimeoutSeconds` is 10.0 and I did not touch it.** Verified at the artifact after the last
   edit: `SiegeLlamaSubsystem.h:161 static constexpr double HardTimeoutSeconds = 10.0;`. Same for
   `MaxOutputTokens = 96` (`:169`), `SoftTimeoutSeconds = 4.0` (`:121`), `UBatchTokens = 64` (`:240`),
   `BatchTokens = 512` (`:243`), `VramGameReserveMiB = 768` (`.cpp:104`). **Fence 1 held.**
2. **The header was NOT softened.** The `BACKSTOP` word survives on Partial/FullOffload. What changed
   is *what makes it one* — the loop, not the callback — because **the code now implements it.**
   **Fence 2 held, in the required direction.**
3. **⚠️ THERE IS NO `git diff` FOR THIS TASK AND THAT IS A FACT ABOUT THE REPO, NOT A GAP IN THE
   REPORT.** Both files are **UNTRACKED** (`git ls-files --error-unmatch` errors on both). There is no
   baseline to diff against, so **§18a's "account for every line at its new home" is discharged by the
   itemisation in §6 below, not by an arithmetic delta.** Which is the shape §18a asks for anyway.
4. **⛔ I did not compile, did not run the editor, did not touch Git, did not touch `Content/`, and did
   not touch 🔒 `SiegeLlamaSpike.cpp` (§16).** Everything below marked *unverified without a compile*
   is flagged as such in §8.
5. **I found and fixed a THIRD over-claim of the same shape, and I am declaring it rather than
   burying it** — the *"at most one ubatch"* granularity figure, which was wrong by **8x** in the
   flattering direction, in **both** this code and `handoffs/TASK-450-programmer.md`. See §3.

---

## 1. THE DEFECT, AND WHAT NOW ENFORCES THE DEADLINE ON WHICH TIERS

### Before

The prefill loop's between-slice test was `if (bCancelRequested || bStopRequested)` — **no deadline
term.** The deadline reached the prefill by exactly **one** route:

```
AbortThunk  ->  ShouldAbortNow()  ->  AbortDeadlineUsec
```

and `llama.h:384` documents that route as *"currently works only with CPU execution."*

⇒ On `tier=partial` and `tier=full` a runaway **prefill** had **no 10 s enforcement whatsoever** until
the decode loop's own check — which is on the *far side* of the prefill. The header at
`SiegeLlamaSubsystem.h` asserted a backstop the code did not provide, **in the place a reviewer goes
to check.** That is §17's shape exactly: a claim about the call, not about the world.

### After — the enforcement matrix, by tier and by phase

| phase | CpuOnly | Partial | FullOffload |
|---|---|---|---|
| **prefill** | `abort_callback` **mid-graph** (`llama.h:382-385`) **+** the new between-slice test | ⬅ **NEW: the between-slice test is the enforcement** | ⬅ **NEW: same** |
| **decode** | `abort_callback` mid-graph **+** the per-token test (pre-existing, `Now > HardDeadlineSeconds`) | per-token test | per-token test |

**What enforces it:** `FSiegeLlamaWorker::RunRequestUnguarded`, a plain
`FPlatformTime::Seconds() > HardDeadlineSeconds` on the worker's own thread. **It reads a clock, not a
device** — which is precisely why it is tier-independent and why the guarantee is now true on tiers
where `abort_callback` is not a deadline mechanism at all.

**On which tiers:** **all three.** That is the whole deliverable.

### ⚖️ THE PLACEMENT IS THE PART TO REVIEW, NOT THE CONDITION

I put the new test at the **BOTTOM** of the prefill loop body, not the top. **A top-of-body test would
have looked correct and done nothing on the hot path.** From turn two the prefill is **~347 tokens**
against a **512-token `n_batch`** (CONVENTIONS §8, `DROP = 77.1 %`), so the loop runs its body
**exactly once** and a top-of-body test would never fire — the deadline would first be consulted after
a token had already been sampled in the *decode* loop.

⛔ **QA should check this specifically.** A reviewer confirming "the deadline is in the between-slice
test" without checking *where* would pass a version that is inert on the common path.

### ⚖️ A SEPARATE BRANCH, NOT AN EXTRA TERM — AND THE REASON IS THE PLAYER'S

QA's suggested fix was *"add `|| ShouldAbortNow()` to `:1344`"*. **I did not do that, and I am
declaring the departure with its mechanism (§15).**

Folding the deadline into the existing test sets `bCancelled = true`, and the completion string is
built as `bCancelled ? FString::Printf(TEXT("cancelled -- %s"), ...) : FailureDetail`. ⇒ **A run that
timed out would complete with the word "cancelled", and the FSM would tell the player their order was
cancelled when nobody cancelled it.** Same exit, different fact.

⇒ The new branch sets **`bAborted`**, which is **the same flag the decode loop's hard timeout already
uses** for the same event — so the two timeouts are now expressed identically, and the completion
carries `HARD TIMEOUT during PREFILL: ...` rather than a false cancel.

---

## 2. ⛔ THE SECOND-ORDER BUG THE ONE-LINE FIX WOULD HAVE CREATED — READ THIS ONE

Setting `bAborted` alone is **not sufficient**, and a fix that stopped at the new branch would have
been **worse than the defect it closed.**

The post-prefill guard read `if (bCancelled || bDecodeFailed)`. The new timeout exit sets **neither**.
⇒ It would have **fallen straight through into the sampler and the decode loop** with the prompt only
**partially prefilled**, and `llama_sampler_sample` would have sampled from the logits of **the last
slice that landed** — generating a fluent, grammar-valid continuation of a **truncated prompt**.

⚠️ **This assistant ORDERS UNITS.** A syntactically perfect command derived from half a prompt is the
valid-shaped-wrong-answer failure CONVENTIONS §1 is built around, and it is strictly worse than the
refusal the caller gets instead. **The truncated-JSON hazard §12d names would have moved from the
output to the input.**

⇒ Guard extended to `if (bCancelled || bDecodeFailed || bAborted)`.

**AND IT IS PROVABLY ADDITIVE IN THE §18a SENSE — the behavioural sense, not the arithmetic one.**
Before this task, `bAborted` was set in **exactly one place** in the prefill loop (the
`llama_decode != 0` branch) and **always alongside `bDecodeFailed = true`**, so it could never be the
term that decided this test. **Every pre-existing path through this guard evaluates identically.** The
prefill hard timeout is the first exit in the file's history that sets `bAborted` alone.

---

## 3. ⚠️ DECLARED FINDING — THE "ONE UBATCH" CLAIM WAS WRONG BY 8x, IN THE FLATTERING DIRECTION

Not on the task list. Found while writing the fix, in the exact lines the fix touches. **Declared
rather than buried (§15's second half).**

**Three artifacts claimed the between-slice check bounds a GPU-tier abort to "at most one ubatch"
(64 tokens):**

- `SiegeLlamaSubsystem.cpp`, the prefill loop comment;
- `SiegeLlamaSubsystem.h`, `CancelActiveRequest`'s doc;
- `handoffs/TASK-450-programmer.md` §4, *"bounds a GPU-tier cancel to at most one `n_ubatch` of 64
  tokens."*

**THE MECHANISM, CHECKABLE WITHOUT TRUSTING ME:** the loop calls `llama_decode` with
`ChunkTokens = min(EffectiveBatchSize, remaining)`, and `EffectiveBatchSize = llama_n_batch(Context)`
(assigned at `.cpp` in `LoadModelUnguarded`, **512 requested**). `UBatchTokens = 64` is the size of the
graphs **llama builds internally inside one `llama_decode` call**; the worker **never returns between
them.** ⇒ **A loop cannot bound anything at a boundary it does not return through.**

- **The real bound is ONE `n_batch` SLICE** — up to 512 prompt tokens, i.e. **8 ubatches** — during
  prefill, and **exactly one token** during decode (that loop calls `llama_decode` with a batch of 1).
- ✅ **The GUARANTEE survives — bounded, not unbounded.** Only the number was wrong.
- ⛔ **It was wrong in the direction that flatters the design**, which is why it is worth the lines.
  This is the §12-family shape again: a correct conclusion resting on a figure that belongs to a
  different unit of work.

**Corrected in both files I own.** ⛔ **The TASK-450 handoff is NOT mine to edit and I did not touch
it** — it is recorded here and in the code comments so the next reader reconciles them correctly.

📌 **AND A NON-FINDING, RECORDED SO NOBODY FILES IT:** `SiegeLlamaSpike.cpp:2262-2267` carries similar
wording — but the spike calls its GPU ceiling **"ADVISORY"**, not "BACKSTOP", and says
*"MaxOutputTokens is the real bound"*, which is **accurate for the spike** (the spike has its own
loops and I did not change them). ⇒ **The spike is not over-claiming; the shipped subsystem is simply
now STRONGER than the spike, so the two legitimately differ.** 🔒 §16 — **file untouched, read only.**

---

## 4. WHAT THE HEADER CLAIMS AFTER MY CHANGE, AND WHY IT IS NOW TRUE

`SiegeLlamaSubsystem.h`, `HardTimeoutSeconds` doc. **The `BACKSTOP` promise on Partial/FullOffload
STANDS.** What I added is the mechanism and the granularity:

| the claim | why it is true now |
|---|---|
| CpuOnly: **AUTHORITATIVE**, cuts mid-graph | unchanged — `abort_callback`, `llama.h:382-385` |
| Partial/Full: **BACKSTOP** | **NEW:** the worker's between-slice + per-token tests, which read a clock and are tier-independent |
| "abort_callback is not a deadline mechanism on those tiers **at all**" | `llama.h:384`, quoted verbatim in the file |
| overshoot is **bounded by one `llama_decode` call** — ≤ `llama_n_batch(ctx)` prompt tokens in prefill, exactly 1 token in decode | the loop structure, readable in `RunRequestUnguarded` |
| ⛔ **NOT claimed:** a mid-graph cut on the GPU tiers | stated as an explicit non-claim in the comment |

⚖️ **THE HONEST SCOPE, STATED RATHER THAN QUIETLY OMITTED, per the fence:**

1. **The mid-graph cut genuinely cannot hold on Partial/FullOffload**, because `abort_callback` is
   documented CPU-only. **That is a property of the vendored library, not of this code**, and no
   amount of loop work can produce it. The header now says so in terms and does **not** dress the
   slice-boundary cut up as a mid-graph one.
2. **The clock starts on the worker, not at the keypress.** `HardDeadlineSeconds` is armed from
   `StartSeconds`, captured at the top of `RunRequestUnguarded` — so the wake + handoff latency ahead
   of it is **outside** the ceiling. It is a wake on an already-triggered `FEvent`, so it is small,
   but **this is a bound on INFERENCE time, not on the player's end-to-end wait**, and I wrote that
   distinction into the header rather than letting the two be read as one claim.
3. **`MaxOutputTokens`' doc moved too, and it moved UP, not down.** It read *"THE PRIMARY BOUND ON THE
   GPU TIERS. The deadline is only the backstop there."* It now names the axis: `MaxOutputTokens`
   bounds a run's **length**, the deadline bounds its **duration**, and the duration bound now holds
   on every tier. ⛔ **No value changed** (96).

---

## 5. WARN-1 DISPOSITION — FIXED, NOT JUSTIFIED, AND IT IS ABOUT TO STOP BEING INERT

**Three coupled changes; none is safe without the other two.**

| # | site | change |
|---|---|---|
| a | `MarkBusy()` | now **refuses** unless `GetState() == EState::Idle` — symmetric with `ClearBusy()`, which has always refused to clear `Faulted`/`Stopped` |
| b | `SubmitRequest()` | `MarkBusy()` **moved to BEFORE** `bWorkPending = true` |
| c | `Run()`, the `bWorkPending` branch | re-tests `GetState() == EState::Busy` before dispatching; **completes the request on the else branch** |

### ⛔ (b) IS NOT TIDYING — IT IS WHAT MAKES (a) SAFE, AND I AM DECLARING IT

QA's suggested fix was **(a) + (c) only, "two lines"**. **Those two alone introduce a NEW race on the
HEALTHY path**, and this is the mechanism:

> The run loop's wait is **bounded at 200 ms**, so the worker ticks even without a trigger. With
> `bWorkPending = true` executing **before** `MarkBusy()`, the worker can wake in that gap, observe
> pending work, find the state still **Idle** — because the game thread has not reached `MarkBusy()`
> yet — and **refuse a perfectly healthy request.**

Publishing `Busy` first collapses the window: **`bWorkPending == true` now implies `MarkBusy()` has
already run.** The window becomes empty by construction rather than merely narrow.

📌 **AND THE ORDER WAS ALREADY THE DOCUMENTED ONE.** `SubmitRequest`'s own comment has always read
*"1. publish the payload, 2. flip Idle -> Busy, 3. wake"* — and the code did 3-before-2 and dispatched
off a flag the comment never mentions. **The comment described a protocol the code did not implement.
It does now.** ⇒ This is the same §17 shape as WARN-2, one function away, and it is why I judged the
reorder in scope rather than a "threading model change": **the threading model is unchanged** (one
runnable, queue depth 1 by early return, `FEvent` wake, no queue). Two statements swapped inside one
function.

### ⛔ (c)'s ELSE BRANCH IS REQUIRED BY THE PINNED CONTRACT, NOT BY TIDINESS

`RequestCompletion`'s doc: *"a true return means OnComplete fires exactly once, later, on the game
thread, whatever happens."* If `MarkBusy()` refuses, `RequestCompletion` has **already returned true**.
Dropping the request would **strand the FSM in "thinking" for the rest of the match with no error, no
log and no crash.** So the else branch drains the payload under the lock, logs at Warning, and calls
`PostCompletion(false, ..., reason, false)`. **The contract holds through the new path.**

### ⚠️ WARN-1 STOPS BEING INERT WHEN TASK-455 LANDS — THIS IS THE PART TO CARRY FORWARD

QA scoped it *"Inert today (no game-lane caller)."* **TASK-455 adds one, and it adds exactly the
caller that reaches the window:** it registers the static prefix and submits the request **in the same
frame**, relying on the run loop's `ApplyPendingStaticPrefix()` → `bWorkPending` order
(`SiegeAssistantComponent.cpp`, the static-prefix backstop comment; `handoffs/TASK-455-programmer.md`
§5d). That path runs `ApplyPendingStaticPrefix()` → `RunBudgetAssertion()` → **`LatchFaulted()`** — the
one worker transition out of `Idle` — **on the very iteration that then dispatches.**

- **Before:** the request would decode on a session that had already declared itself faulted.
- **After:** it is refused, with a completion the FSM can act on.

⚠️ **Honest bound on that claim:** `LatchFaulted` there fires only on `Total > n_ctx_actual`, and on
today's measured content `1139 + 400 + 96 + 48 = 1683 < 2048`, so **it will not fire on today's Zone
A.** ⇒ **Reachable by construction with a real caller; not reachable on today's numbers.** I am not
claiming I fixed a live crash.

✅ **CROSS-LANE ORDER PRESERVED, AND QA SHOULD CONFIRM IT:** `ApplyPendingStaticPrefix()` still
precedes the `bWorkPending` branch. **I added a condition INSIDE that branch; I did not move the
branch.** TASK-455's ordering claim survives unchanged, and I wrote that dependency into the code
comment so a later editor does not reorder it.

---

## 6. WARN-3 DISPOSITION — CITATION RE-POINTED, VALUE UNTOUCHED

`SiegeLlamaSubsystem.cpp`, `VramGameReserveMiB`. ⛔ **768 is unchanged** (`.cpp:104`).

- **Was:** *"vram_free_before = 3320.2 -> after = 617.4 MiB, and **bar #1 (frame time) still PASSED on
  p99 in exactly that state**."*
- **The defect:** 617.4 MiB is the **FULL**-tier run (`handoffs/TASK-413-buildmaster.md:638`), while
  **bar #1's result rows are scoped to the PARTIAL tier** — verified at the artifact, **both** rows:
  `:599` and `:1103`. The sentence borrowed a partial-tier bar's authority for a full-tier VRAM state.
- **Now cites `:625`** — *"the p99 attributable delta is **+2.0 to +4.6 ms on every tier**"* — which
  actually covers the tier in question.
- ✅ **The conclusion survives intact.** I added one line saying why a citation fix is worth the space:
  the next reader to re-derive 768 follows the citation and lands on a measurement that cannot support
  it. **The number was never the problem.**

---

## 7. ITEMISED CHANGE LIST (§18a — behaviour, not arithmetic)

⚠️ **No `git diff` exists: both files are UNTRACKED.** Itemised by symbol, per §18c.

### `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`

| # | symbol / site | code lines | behaviour |
|---|---|---|---|
| 1 | `SiegeLlamaPrivate::VramGameReserveMiB` doc | **0** | none — citation text only, WARN-3 |
| 2 | `SiegeLlamaPrivate::AbortRegime()` | **1** (the GPU return string) | log text only; no control flow |
| 3 | `FSiegeLlamaWorker::SubmitRequest()` | **2 REORDERED**, 0 added, 0 removed | `MarkBusy()` now precedes `bWorkPending = true` |
| 4 | `FSiegeLlamaWorker::MarkBusy()` | **+3** | refuses unless `Idle` |
| 5 | `FSiegeLlamaWorker::Run()`, `bWorkPending` branch | **+~20** | state re-test + refusal completion; `RunRequestGuarded()` itself unchanged, now nested |
| 6 | `RunRequestUnguarded()` "ARM THE DEADLINE" | **0** | comment only |
| 7 | prefill loop, top-of-body comment | **0** | comment only — the granularity correction (§3) |
| 8 | prefill loop, `DecodeResult == 2` detail string | **1** (string literal) | stops asserting *"the cancel"* when the deadline and shutdown feed the same callback |
| 9 | prefill loop, **bottom of body** | **+9** | ⭐ **THE FIX** — deadline test, sets `bAborted`, distinct failure detail |
| 10 | post-prefill guard | **1** (`\|\| bAborted`) | catches item 9's exit; inert on every pre-existing path (§2) |

### `Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h`

⛔ **COMMENT-ONLY. ZERO DECLARATIONS CHANGED — verified verbatim after the last edit:**
`HardTimeoutSeconds = 10.0` (`:161`) · `MaxOutputTokens = 96` (`:169`) · `IsReady()` (`:257`) ·
`IsBusy()` (`:260`) · `RequestCompletion(...)` (`:272`) · `CancelActiveRequest()` (`:302`).
**§9's pinned registry is byte-intact.**

| # | doc block | change |
|---|---|---|
| 11 | `HardTimeoutSeconds` | the backstop claim, now with its mechanism, its granularity, and its two explicit non-claims |
| 12 | `MaxOutputTokens` | names the axis (length vs duration); no softening |
| 13 | `CancelActiveRequest` | the "one ubatch" correction (§3) |

### Scope fence — what I did NOT do

⛔ No `HardTimeoutSeconds` change · no queue · no threading-model change · no game-lane edit · no
`Content/` · no Git · no compile · no editor/MCP/PIE · 🔒 **`SiegeLlamaSpike.cpp` UNTOUCHED** (§16 —
read only, in §3). **`SiegeLlamaInfo.cpp`, `SiegeLlamaModule.cpp`, `SiegeLlamaSettings.cpp`,
`SiegeLlamaSettings.h`, `SiegeLlamaLog.h`, `SiegeLlamaModule.h`, `SiegeLlama.Build.cs` all untouched.**

---

## 8. ⚠️ WHAT IS NOT VERIFIABLE WITHOUT A COMPILE (TASK-447's gate)

1. **That it compiles.** New code is small and uses only idioms already in the file
   (`FPlatformTime::Seconds()`, `FString::Printf` with a **literal** format string per the
   `TCheckedFormatString` trap, `FScopeLock`, `UE_LOG`). `MarkBusy`'s body change keeps its `void`
   signature. **Balance checked mechanically: braces 215/215 and parens 931/931 in the `.cpp`; zero
   non-ASCII bytes in either file; every `*/` is a bare terminator or a `/*param*/` marker** (the §10
   comment-terminator trap).
2. ⛔ **That the prefill timeout ever FIRES.** It needs a prefill that exceeds 10 s. **Measured prefill
   is ~1.7 s on partial** (`TASK-413`), so **this will not fire in normal play and TASK-447 cannot
   observe it without forcing it.** ⇒ **The reachable test is the CPU tier**, where TASK-413 measured
   **2 of 5 generations hitting the ceiling**. ⚠️ **Absence of the new log line is NOT evidence the fix
   works** — it is evidence the prefill was fast. **Do not score this by silence.**
3. **That the WARN-1 refusal branch ever executes.** It needs `LatchFaulted` during
   `ApplyPendingStaticPrefix` in the same iteration as a dispatch — **not reachable on today's Zone A**
   (§5). Its correctness is a reading argument, not a runtime one.
4. **The exact overshoot in milliseconds.** I state the bound **in units of work** (one `llama_decode`
   call, ≤ `llama_n_batch(ctx)` tokens) and deliberately **do not convert it to seconds** — that would
   be a derived delta from a single run, which **§12h forbids.**

---

## 9. §14 DISCHARGE — WITH THE POSITIVE CONTROL, RUN BEFORE ANY NEGATIVE WAS TRUSTED

I claim *"`MarkBusy()` has exactly one caller"*. **Here is the proof the search could have found a
second one, per the TASK-454 pattern the law pins:**

```
$ git ls-files --error-unmatch Plugins/.../SiegeLlamaSubsystem.cpp
error: pathspec '...' did not match any file(s) known to git   <- UNTRACKED

$ git grep -n "ShouldAbortNow" -- .
(no output)                                                     <- BLIND

Grep (working tree) "ShouldAbortNow"  ->  .cpp:365, :371, :513   <- the symbol IS there
```

⇒ ⛔ **`git grep` is provably blind to this file. A negative from it would have meant nothing**, and I
used the working-tree `Grep` tool for every search in this task. That same working-tree search
**positively found** `MarkBusy` at its definition and at its single call site in `SubmitRequest`, and
`ClearBusy` at its definition and its single call site in `HandleWorkerCompletion` — so the
one-caller claim rests on a search **demonstrated capable of finding callers**, not on a bare zero.

📌 Structural/comment-level findings in this task were read from **raw `Read` output**, never from
`Grep` output (§10 / §14 instance 1 — `Grep` mangles `//`).

---

## 10. WHAT QA SHOULD SCRUTINISE

1. ⭐ **The PLACEMENT of the new deadline test — bottom of the prefill loop body, not top.** A
   top-of-body version is inert on the hot path (one slice from turn two). **This is the check most
   likely to pass a broken fix.**
2. ⭐ **The `|| bAborted` on the post-prefill guard.** Without it the new exit falls into the sampler
   with a half-prefilled prompt (§2). **Confirm my "provably inert on pre-existing paths" argument:
   is `bAborted` set anywhere else in that loop without `bDecodeFailed`?**
3. **That a timeout does not report as a cancel.** Trace `bAborted` (not `bCancelled`) → the
   `bCancelled ? "cancelled -- ..." : FailureDetail` ternary.
4. ⛔ **Rule on the `MarkBusy` / `bWorkPending` REORDER (§5b).** It exceeds QA's literal "two lines".
   **The mechanism is the run loop's 200 ms bounded wait**, and the reorder is what stops the guard
   from creating a healthy-path race. If you judge it out of scope, say so — the reorder is separable
   from the guard, but **the guard is NOT safe without it.**
5. **The cross-lane ordering:** `ApplyPendingStaticPrefix()` still precedes the `bWorkPending` branch.
   TASK-455 depends on it.
6. **§3's granularity correction** — 8x, in the flattering direction, and it contradicts a **shipped
   handoff** (TASK-450 §4) that I am not allowed to edit.
7. **WARN-3:** confirm `768` is unchanged and that `:625` is quoted correctly.
8. **Verify by symbol, not offset (§18c).** Every line number in `qa/TASK-424.md` and in the TASK-457
   spec is now stale — this task added ~200 lines to the `.cpp`. `:1344` is not where the between-slice
   test lives any more, and **a criterion that fails only because a number moved has found nothing.**

---

**Status:** `ready-for-qa`. Review folds into **TASK-446** as a scoped plugin appendix, **cross-lane**.
