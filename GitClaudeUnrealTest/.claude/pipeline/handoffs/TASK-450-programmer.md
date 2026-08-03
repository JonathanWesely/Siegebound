# TASK-450 — gameplay-programmer handoff (2026-08-03)

**M8 DECLARATION DUTY, stated verbatim as required:**
> "adds no replicated property, no new replicated class, no new relevancy tier."

---

## ⛔ READ THIS FIRST — FOUR THINGS THIS HANDOFF WILL NOT LET YOU BELIEVE

1. **NOTHING HERE HAS BEEN COMPILED OR RUN.** No compile, no editor, no MCP, no
   PIE (dispatch instruction; TASK-447 owns the one gate). **Every claim below is
   a claim about code, not an observation of behaviour.** §6 is the honest table
   of what TASK-447 can and cannot actually verify.
2. **THE SPIKE HARNESS IS STILL PRESENT — DEFERRED AGAIN, AND ON A *DIFFERENT*
   REASON THAN TASK-423'S.** TASK-423's reason ("no subsystem ⇒ zero load
   paths") is **discharged** — the subsystem exists. A newer, decisive one
   replaced it. §7.
3. **SPEC ITEM (9) IS ONLY HALF MINE AND THE OTHER HALF IS IN THE GAME LANE.**
   `MaxSnapshotChars`'s retirement and `ZoneBCharReserve`'s re-measurement live
   in `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h`, which a
   plugin-scoped task may not touch. §8.
4. **THERE IS NO GAME-LANE CALLER YET, SO TWO OF THE THREE BUDGET GUARDS ARE
   INERT TODAY** — and the code says so out loud, once per session, at Warning,
   rather than looking like a pass. §5.

---

## 1. What was delivered

**Two new files. No edits to any existing file, anywhere in the repo.**

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Plugins\SiegeLlama\Source\SiegeLlama\Public\SiegeLlamaSubsystem.h` (329 lines)
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Plugins\SiegeLlama\Source\SiegeLlama\Private\SiegeLlamaSubsystem.cpp` (2046 lines)

`git status` confirms exactly two `??` entries under `Plugins/`. **Zero game-lane
files, zero `Content/`, zero Git, zero compile.** The plugin is a separate UBT
module, so this is genuinely parallel-safe against the game-module lane.

**§9 pinned registry, verified character-for-character against
`CONVENTIONS.md:827-839` by grep, access levels included:**

```cpp
DECLARE_DELEGATE_ThreeParams(FSiegeLlamaCompletionSignature, bool /*bSuccess*/, const FString& /*Output*/, const FString& /*Error*/);
bool IsReady() const;
bool IsBusy() const;
bool RequestCompletion(const FString& Prompt, const FString& Grammar, const FSiegeLlamaCompletionSignature& OnComplete);
void CancelActiveRequest();
```

`LogSiegeLlama` is **included** from `SiegeLlamaLog.h`, never re-declared and
never re-defined — TASK-409's standing instruction, and a second
`DEFINE_LOG_CATEGORY` would be a duplicate-symbol **link** error that surfaces
late. Grep confirms zero `DECLARE_/DEFINE_LOG_CATEGORY` in either new file.

**Tunables landed as named constants (§10), all flagged for Jonathan's feel pass:**
`SoftTimeoutSeconds` 4.0 · `HardTimeoutSeconds` 10.0 · `MaxOutputTokens` 96 ·
`MaxSnapshotTokens` **400** · `SnapshotPreFilterMaxChars` **3000** ·
`ContextTokens` **2048 (FROZEN)** · `SafetyMarginTokens` **48** ·
`UBatchTokens` 64 · `BatchTokens` 512.

They are `static constexpr` rather than `EditDefaultsOnly` **deliberately**: a
`UGameInstanceSubsystem` has no editable asset, so an `EditDefaultsOnly` property
here would be invisible in the editor and would merely *look* tunable. §10
permits either form. The game lane reads these symbols directly, so there is
exactly one copy of every number.

### The two dead spellings — verified at the artifact, not taken from the spec

| spelling | vendored header says |
|---|---|
| `llama_memory_seq_rm` | ✅ **`llama.h:735`** — used |
| `llama_kv_cache_seq_rm` | ⛔ **0 hits across the whole `include/` tree** |
| `load_mode = LLAMA_LOAD_MODE_MMAP` | ✅ **`llama.h:207`** — used |
| `use_mmap` | ⛔ **0 hits across the whole `include/` tree** |

`grep -rn` over `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/include`. The two
dead spellings appear in my file **only inside comments that say they are dead.**

Every other header citation in the code was checked the same way and **two were
wrong in my first draft and are corrected**: the progress-callback doc is
`llama.h:323-325` (I had written 324-325) and `llama_decode`'s return codes are
`llama.h:971-975` (I had written 963-977, which is the surrounding prose).

---

## 2. THE THREAD-OWNERSHIP ARGUMENT (owed by the spec, stated as an argument)

**The requirement is that exactly one thread ever touches `llama_context`. The
question is not whether a design *can* satisfy that — several can — but whether
it satisfies it STRUCTURALLY or by CONVENTION.**

A task-graph or `Async()` design satisfies it by convention: *"we only ever
submit one at a time"*, *"the FSM serialises them"*. That is a **comment**,
enforced by nobody, in a repo where eight tasks run in parallel and any of them
can add a second submission site without touching the file that holds the
invariant. The task graph additionally **cannot** give the guarantee even in
principle — its threads are shared, a task may resume on a different worker, and
a multi-second CPU burn does not belong in a pool the renderer also uses.

**What was built instead:** one `FSiegeLlamaWorker : FRunnable`, created in
`Initialize()` at `TPri_BelowNormal`, alive for the subsystem's whole lifetime.
`Model`, `Context`, `Vocab` and `LastPromptTokens` are **private members of that
object**. No other object holds them. Every function that dereferences them is a
member of that class and runs on `Run()`. The subsystem holds a pointer to the
*worker*, never to the context.

⇒ **There is no path by which a second thread reaches `llama_context` without
adding a new member function AND calling it from off the run loop — a visible,
reviewable act rather than an omission.**

**The ownership map is written into the header comment so a reviewer can CHECK it
by reading rather than by reasoning:**

| owner | what |
|---|---|
| **GAME THREAD ONLY** | `Initialize` · `Deinitialize` · `RequestCompletion` · `CancelActiveRequest` · `SetStaticPrefix` · `IsReady` · `IsBusy` · `ActiveDelegate` and its execution · the console commands · `USiegeLlamaSettings::ResolveModelPath` · `FSiegeLlamaModule::EnsureBackendsLoaded` |
| **WORKER THREAD ONLY** | `Model` · `Context` · `Vocab` · `LastPromptTokens` · `StaticPrefixTokenArray` · **every `llama_` and `ggml_` call** except the two callbacks |
| **ANY THREAD (atomic)** | `State` · `bCancelRequested` · `bStopRequested` · `bWorkPending` · `AbortDeadlineUsec` · the `RequestLock`-guarded handoff slot |
| **GGML COMPUTE THREADS** | `AbortThunk` only — three atomic reads, nothing else |

**The completion delegate never crosses the boundary at all.** The worker posts a
plain `(bool, FString, FString)` to the game thread; `ActiveDelegate` is assigned,
read and executed **only** on the game thread. That is what makes *"OnComplete
ALWAYS fires on the GAME THREAD"* a property of the structure rather than a
promise. It holds on **every** path: success, model-missing, budget rejection,
prefill abort, decode abort, hard timeout, cancellation, and SEH fault.

**Two flags drive the handoff, on purpose.** `bWorkPending` means *"the worker has
something to do"* and the **worker** clears it; `State == Busy` means *"a request
is in flight"* and the **game thread** clears it when the completion lands.
Driving the run loop off `Busy` alone would re-enter the same request in the
window between `PostCompletion` and the game thread reaching `ClearBusy` — and
would re-enter it with an already-moved-from payload. **This was a real bug in my
first draft, found by re-reading the loop rather than by a tool.**

---

## 3. THE TIERING POLICY, WITH THE NUMBERS IT CAME FROM

**Source: `handoffs/TASK-413-buildmaster.md`, `Qwen3-4B-Q4_K_M` at `ctx=2048`, one
pinned RTX 5070 Laptop (7891 MiB total).** Every tier run twice, the second
reported.

| tier | `n_gpu_layers` | model-attributable `vram_delta` | mean TTFT | mean wall | ms/token | aborts |
|---|---|---|---|---|---|---|
| cpu | 0 / 36 | 366.7 MiB *(Vulkan scratch; weights in RAM)* | 2716 ms | 8730 ms | 156-182 | **2 of 5** |
| partial | **18 / 36** | **793.5** then **1264.0** MiB | 1655 ms | 6472 ms | 117-131 | 0 of 5 |
| full | 36 / 36 | **2702.8** then **2701.8** MiB | 417 ms | ~2100 ms | 36-49 | 0 |

**The policy, in the code as three thresholds:**

```
FullOffloadCostMiB    = 2703   MEASURED twice, 1 MiB apart
PartialOffloadCostMiB = 1264   MEASURED twice -- the WORSE of the two is used
VramGameReserveMiB    =  768   derived from a measured operand, see below

free >= 2703 + 768 = 3471  ->  FULL     (n_gpu_layers = -1)
free >= 1264 + 768 = 2032  ->  PARTIAL  (n_gpu_layers = 18)
otherwise                  ->  CPU      (n_gpu_layers = 0)
```

**Four things about those numbers that a summary would lose:**

1. **`VramGameReserveMiB = 768` is derived from a measurement, not eyeballed.**
   The full-offload run went `vram_free_before = 3320.2 -> after = 617.4 MiB`,
   and **bar #1 (frame time) still PASSED on p99 in exactly that state.** So
   ~617 MiB was *empirically survivable on this content*. 768 is that survivable
   floor rounded up with margin. It is not a general claim about what a GPU needs.
2. **The partial tier's own number has a 1.6× spread** (793.5 vs 1264.0 MiB, same
   configuration, two sessions). §12h: the spread travels with the number, and a
   bound is sized from the **worse** reading. It is.
3. **On TASK-413's own machine this policy selects PARTIAL** — 3320.2 MiB free is
   below the 3471 full-offload threshold. That is the tier the frame-time bar was
   written against and the board's stated default, reached from the arithmetic
   rather than hard-coded to it.
4. ⛔ **The CPU tier MEASURABLY FAILS bar #2** — 2 of 5 generations hit the 10 s
   ceiling with **truncated JSON**. It is still selectable, because Jonathan's
   ruling 1 is that CPU fallback is *required* (no vendor lock-in) — but
   selecting it logs a **Warning that names the measurement**, so a slow session
   reads as a known tier property rather than a new bug.

**Device pinning:** exactly one device, from an explicit one-entry `devices` list
with `LLAMA_SPLIT_MODE_NONE` and `main_gpu = 0`. `llama.h:307` documents `devices`
as *"if NULL, all available devices are used"* and `split_mode` defaults to
LAYER — so the default lets the weights straddle two cards while a memory sample
reports the free mark of one. **A tier must mean one named device.** Every
enumerated device is logged with free/total/description before the load.

⚠️ **The `18` is `18-of-36` and nothing else.** If `llama_model_n_layer()` comes
back ≠ 36 the code logs a Warning that the partial-tier measurements **do not
describe this model** — an arbitrary fraction of a different layer count is not
the configuration those numbers were taken on.

⚠️ **The iGPU case is handled but not trusted.** TASK-413 measured one reporting
`free (47690 MiB) > total (37020 MiB)` — an incoherent pair. An iGPU is pinned if
it is all there is, but its headroom reading is **never allowed to promote a
tier**; the tier is held at PARTIAL with the reason logged.

---

## 4. THE ABORT PATH, TRACED END TO END (owed by the spec)

```
GAME THREAD                WORKER THREAD                 GGML COMPUTE THREADS
-----------                -------------                 --------------------
CancelActiveRequest()
  check(IsInGameThread())
  if !IsBusy() -> return        (idempotent, silent)
  log + regime label
  Worker->RequestCancel()
    bCancelRequested = true   ---------------------------------->
                                                          AbortThunk(void*)
                                                            ShouldAbortNow()
                                                              bCancelRequested? -> TRUE
                                                          llama_decode ABORTS
                                                          (returns 2)
                             prefill/decode loop sees
                             DecodeResult == 2
                               bAborted = true
                               bCancelled = bAborted && bCancelRequested
                               LastPromptTokens.Reset()   <-- cache state unknown
                               AbortDeadlineUsec = 0
                               PostCompletion(false, "", "cancelled -- ...")
                                 AsyncTask(GameThread, ...)
HandleWorkerCompletion  <-------------------------------------------
  ClearBusy()   (Busy -> Idle, BEFORE the delegate)
  Delegate.ExecuteIfBound(false, "", "cancelled -- ...")
```

**Three separate mechanisms feed `ShouldAbortNow()`, and they are not
interchangeable:**

| trigger | what it is |
|---|---|
| `bCancelRequested` | `CancelActiveRequest()` |
| `bStopRequested` | `Deinitialize()` — a shutdown aborts an in-flight decode by the same route |
| `AbortDeadlineUsec` | the 10 s hard ceiling, armed per request, disarmed on every exit |

⚠️ **THE REGIME HONESTY, AND IT IS THE PART THAT MUST NOT BE FLATTENED.**
`llama.h:382-385`, verbatim: *"Abort callback / if it returns true, execution of
`llama_decode()` will be aborted / **currently works only with CPU execution**."*

- **`tier=cpu`** — the callback is polled **inside** `llama_decode`, so a cancel
  lands **mid-graph, genuinely mid-prefill**, and the hard deadline is a
  **promise**.
- **`tier=partial` / `tier=full`** — the callback can only be consulted **between
  graph submissions**. A submission already in flight runs to completion. The
  cancel lands at the next boundary and the deadline is a **backstop**;
  `MaxOutputTokens = 96` is the primary bound.

**So the design does not rely on `abort_callback` alone.** Both loops
additionally test `bCancelRequested || bStopRequested` **between slices** — which
bounds a GPU-tier cancel to **at most one `n_ubatch` of 64 tokens**, not an
unbounded wait. `AbortRegime(Tier)` is printed on every line that mentions the
deadline, so nobody compares a cpu wall time against a full-offload wall time at
the same ceiling and concludes something false about the knob.

⇒ **What I can honestly claim to TASK-447: on `tier=cpu`, cancel aborts
mid-prefill. On the GPU tiers, cancel lands at the next ubatch boundary during
prefill.** If the gate wants the *mid-graph* claim tested, it must run
`Siege.Llama.Test long cancelms=200` **on the cpu tier** and read the
`llama_decode=2` line; on a GPU tier the same command proves the boundary path,
which is a different (weaker, and correctly-labelled) claim.

**`LastPromptTokens.Reset()` on every abnormal exit** is load-bearing: after an
abort the KV cache holds an unknown number of the aborted turn's tokens, so the
next turn must not claim a prefix it cannot prove. Keeping it would report a
large, entirely fictional reuse figure.

---

## 5. THE THREE BUDGET GUARDS — AND WHICH ARE INERT TODAY

### 5a. The startup budget assertion (§8 ZONE-A SIZE BOUND, ruling 2 — MANDATORY)

```
ZoneA_tokens + MaxSnapshotTokens + MaxOutputTokens + SafetyMarginTokens <= llama_n_ctx(ctx)
```

- **`ZoneA_tokens` is `llama_tokenize`d from the registered prefix string at the
  instant the assertion runs.** Never the recorded 1139, never any constant. The
  clause's own argument is the reason: §9a *sanctions* widening the place
  vocabulary, widening it grows Zone A, and a `1139` literal is an operand that
  **cannot grow** — so the assertion would pass forever, and pass most
  confidently at the exact moment it should fail.
- **`n_ctx_actual` is `llama_n_ctx(ctx)`,** never the requested 2048.
  `llama.h:551-556` says the context is entitled to use a different value, and
  requested-vs-actual is printed **side by side, once**, with a `<== CLAMPED`
  marker, for `n_ctx`, `n_batch` and `n_ubatch`.
- **It re-runs on every prefix change**, which is precisely the growth path it
  exists to catch.
- **Failure is loud, at load, and names which operand overflowed** — never at the
  first sentence of a match, never a silent truncation.

**`SafetyMarginTokens = 48`, derived from two measured bands rather than chosen:**
the chat-template wrapper measures **~13 tokens** (`turn1_prompt 1517` vs
`assembled_total 1504`), and §12g's tokenisation error band is **−15 to +4 with
unpredictable sign** with its own ruling that anything under **~20 tokens** of
derived headroom is *undecided until measured*. `13 + 20 = 33` is the
floor-of-floors; 48 clears it by ~1.45×. ⛔ **Setting it to 13 would satisfy the
arithmetic and defeat the purpose** — that is the seventh seam's error (treating
the tightest value that clears the known case as the safest one).
Sanity on measured operands: `1139 + 400 + 96 + 48 = 1683 of 2048`, and at §12c's
`zoneA_tok ≤ 1389` ceiling it is `1933 of 2048`. **This margin does not invalidate
§12c's ceiling.**

### 5b. The AUTHORITY cut site — tokens, `MaxSnapshotTokens = 400`

Runs on the worker, after tokenizing. `SnapshotTokens = TotalPromptTokens −
StaticPrefixTokens`. Over budget ⇒ **REJECT + Warning naming both counts.**

### 5c. The PRE-FILTER cut site — chars, `SnapshotPreFilterMaxChars = 3000`

Runs on the **game thread**, before a single token is counted — which is what
makes it cheap. Over ⇒ **REJECT + Warning.**

⚖️ **THE ROLE-CHANGE RE-DERIVATION, DECLARED IN THE SAME BREATH AS THE VALUE, as
§8 requires: 3000 is a PRE-FILTER value and `MaxSnapshotChars`'s 1085 is an
AUTHORITY value, and the safe direction inverts between the two roles.** An
authority is safe assuming the **smallest** plausible chars/token ratio; a
pre-filter must never reject what the authority would accept, so it must assume
the **largest**. Floor: `400 × 3.56 = 1424` (the whole-prompt ratio, the highest
ever measured here). Ruled value **3000 = 400 × 7.5**, ~2× the highest measured
ratio (3.79, Zone A), 2.1× the floor, 3.1× as-built B+C (955 chars). **1085 is
carried into nothing.**

### ⚖️ WHY BOTH CUT SITES REJECT INSTEAD OF TRUNCATING — a declared decision

§8's re-anchored condition binds *any* enforcement of the snapshot budget: *"if
the snapshot reaches the model shorter than it was built, something says so."*

**This subsystem satisfies it by never truncating.** The reason is specific to
this feature, not general tidiness: **the last line of the snapshot is the
player's own sentence.** A tail truncation removes the utterance and leaves a
well-formed prompt *about nothing* — the model then emits a syntactically
perfect, confidently wrong command, and **this assistant orders units.** A
refusal the FSM can say out loud is strictly better than a valid-shaped wrong
command, which is the exact failure §1 is built around.

Rejecting is also **strictly stronger** than the condition asks: nothing ever
reaches the model shorter than it was built, because nothing reaches it at all.
⚠️ **QA should rule on this explicitly** — it is a deliberate reading of a clause
written about truncation, by a component that chose not to truncate.

### ⛔ 5d. TWO OF THE THREE ARE INERT TODAY, AND THE CODE SAYS SO

**5b and 5c both need the Zone A / Zone B+C split**, which the plugin can only
know if the game lane registers the static prefix — and **there is no game-lane
caller yet** (that is a later task). So today:

| guard | status today |
|---|---|
| context budget (`PromptTokens + MaxOutputTokens ≤ n_ctx_actual`) | ✅ **ALWAYS enforced, on every request.** There is no unguarded path to `llama_decode`. |
| `MaxSnapshotTokens` authority | ⛔ **INERT** until `SetStaticPrefix` is called |
| `SnapshotPreFilterMaxChars` pre-filter | ⛔ **INERT** until `SetStaticPrefix` is called |
| the startup assertion | ⚠️ **RUNS IN INCOMPLETE FORM** — the context half only, with `ZoneA_tokens` treated as 0 |

**All three inert states log at Warning, once per session, saying exactly what is
not being checked and why.** That is deliberate: *a guardrail that cannot observe
the thing it guards must SAY it cannot, or its silence reads as a pass.* ⛔ **Do
not read a clean log as these budgets passing — read the Warning.**

---

## 6. ⚠️ TASK-447: WHAT MY CODE CAN AND CANNOT SATISFY

**This is the section the dispatch asked for by name. It exists so the gate is
not quietly reduced to whatever happens to run.**

Because **no game-lane caller exists**, I shipped three dev console commands
(`FAutoConsoleCommand`, `Siege.Llama.*` namespace, §5-legal, `#if
!UE_BUILD_SHIPPING`). **Without them the inherited gate has no way to exercise a
single one of these properties.** They are the harness, and they are named in §9
below as a declared addition.

| inherited gate item | can my code satisfy it? | how, and what it actually proves |
|---|---|---|
| `Siege.Llama.Info` runs | ✅ **YES — untouched.** It lives in `SiegeLlamaInfo.cpp`, which I did not modify. | It proves the DLL link, not the subsystem. Run `Siege.Llama.Status` for the subsystem. |
| async load never blocks match start | ⚠️ **PARTIALLY — and the deviation is mine, declared.** | The **2.5 GB weight read is fully async** on the worker; `Initialize()` returns immediately. **BUT `FSiegeLlamaModule::EnsureBackendsLoaded()` is called SYNCHRONOUSLY on the game thread** in `Initialize()`. Reason: the module's `bBackendsLoaded` latch is a plain bool and `Siege.Llama.Info` can call the same function from the console, so doing it on the worker is a data race on state I do not own. It is the **cheap** half (DLL registration, not weights), it runs **only after the model file is confirmed present**, and **it is timed and logged** — `LLM_LOAD: ggml backends ready in N ms`. ⇒ **TASK-447 should report that N.** If it is large, that is a finding for Jonathan, not something to wave through. |
| one parseable JSON from a hard-coded prompt + GBNF | ✅ **YES** | `Siege.Llama.Test`. The probe GBNF is deliberately trivial (`root ::= "{\"ok\":" flag ",\"n\":" digit "}"`) — its job is to prove constrained decoding emits one parseable JSON, **not** to test the real grammar. ⚠️ **Rule names use hyphens/letters only, no underscores** — TASK-413 lost six bench runs to `at_least`. If `llama_sampler_init_grammar` returns NULL the code logs at **Error** and says the result is UNCONSTRAINED and must not be reported as a constrained one. |
| queue depth 1 — a second concurrent call returns false | ✅ **YES, deterministically, in ONE command** | `Siege.Llama.Test twice` issues both calls **in the same function, on the same frame**, and logs both return values. **Pass = call 1 TRUE, call 2 FALSE.** ⛔ **Two FALSEs proves nothing** (it means the model was not loaded) — the log line says so explicitly. |
| `CancelActiveRequest()` aborts mid-prefill within the hard timeout | ⚠️ **YES, WITH A REGIME QUALIFIER THAT MUST BE REPORTED** | `Siege.Llama.Test long cancelms=200`. **On `tier=cpu` this is a genuine mid-graph abort** and the log will carry `llama_decode=2`. **On `tier=partial`/`full` it lands at the next ubatch boundary (≤64 tokens)** — `abort_callback` is documented CPU-only. **Report which tier the run was on.** A cancel measured on `full` is not evidence for the mid-graph claim. |
| a deliberately-absent model file leaves the match fully playable with one log line | ✅ **YES** | Rename the `.gguf`. `Initialize()` logs **exactly one Warning**, starts no thread, registers no backends, and returns. `IsReady()` is false forever. Nothing retries. |
| partial-tier frame-time hitch re-measured vs TASK-413 | ⛔ **NO — I CANNOT SUPPORT THIS AND IT MUST NOT BE FAKED** | **The frame sampler and the hitch histogram live in `SiegeLlamaSpike.cpp`.** My subsystem has **no frame instrumentation at all** — that was never in its spec. ⇒ **The only instrument that can produce a comparable hitch number is the spike**, which is why §7's deferral also protects this gate item. Measuring it with the subsystem would require building a second sampler, and a hitch number taken with a *different* instrument is not comparable to TASK-413's anyway (§12h's sibling problem). **Either run it on the spike, in its own session, or record it as NOT MEASURED.** |
| WARN-5 / `Siegebound.Assistant.ZoneA.*` green | ⛔ **NOT MINE, AND STILL NOT DISCHARGED** | TASK-423's test file is unchanged by me. It has still never run. Discharge is the compile + a green run, claimed against **that run**, never against a handoff. |

**Suggested gate order** (each step's failure is diagnostic for the next):
`Siege.Llama.Info` → `Siege.Llama.Status` → `Siege.Llama.Test` →
`Siege.Llama.Test twice` → `Siege.Llama.Test long cancelms=200` → rename the
`.gguf` → restart → `Siege.Llama.Status`.

⛔ **DO NOT run any `Siege.Llama.Spike*` command in the same session as
`Siege.Llama.Test`.** Two model-load paths exist in this build (§7) and driving
both puts two ~2.5 GB models on one card. **The subsystem logs this warning at
load**, and that warning is deleted with the spike.

---

## 7. ⛔ THE SPIKE DELETION IS DEFERRED AGAIN — ON A NEW REASON, DECLARED

**TASK-423's reason is DISCHARGED.** *"With no subsystem, deleting the spike
leaves zero model-load paths"* no longer applies: the subsystem exists. I record
that plainly because a deferral that recycles a dead argument is how an item
rots.

**A different, newer, and I believe decisive reason replaced it. Three parts, the
first on its own sufficient:**

1. ⛔ **THE SEALED GENERATION-2 HOLDOUT CORPUS WOULD BE SPENT BY DESTRUCTION.**
   It was committed **two commits ago** (`21f7e01`, *"committed exactly as
   authored, UNSPENT, and it must never be edited"*). §12a says it is **authored
   against the FROZEN `t0` fixture in `SiegeLlamaSpike.cpp`** and is scored by
   **`Siege.Llama.SpikeEval`**, which §12a names as the adoption mechanism
   (*"ZERO CODE CHANGE IS REQUIRED TO ADOPT A GENERATION"*), and it is **"SEALED
   at handoff and opened exactly once, by the measurement task."**
   ⇒ **Deleting `SiegeLlamaSpike.cpp` destroys BOTH the fixture the holdout was
   authored against AND the only harness that can ever open it.** A sealed
   artifact the pipeline deliberately created *this week* for a future
   measurement would become unusable, silently, inside a commit whose stated
   purpose is tidiness. **That is spending a holdout without scoring it.**
2. **I could not take a final `zoneA_tok` reading, and the dispatch's ordering
   condition therefore cannot be met inside this task.** *"After any final
   `zoneA_tok` reading has been taken"* — my dispatch also forbids the editor,
   MCP and PIE outright (Jonathan is at the keyboard with the model loaded), and
   `Siege.Llama.SpikePrompt` is the only command that prints one. **Deferring
   does not buy the reading, but deleting destroys the possibility of ever taking
   it.** The asymmetry decides it.
3. **The partial-tier hitch re-measurement (§6) needs the spike's frame sampler**,
   which my subsystem does not have and was never specced to have.

**⚠️ WHAT WEAKENED, HONESTLY, AND IT CUTS AGAINST ME:** TASK-423's second reason
(*"the spike is the only instrument for Zone B's chars"*) is now **wrong**.
`USiegeAssistantComponent::ReportFirstCapture` (game lane, landed, unexecuted)
prints `zoneA_chars / zoneB_chars / zoneC_chars` **from the SHIPPED builder** —
a strictly better instrument for item 9b than the spike's 13-kind fixture. I
found that by reading the artifact and it removes one of the three legs. **I am
reporting it because it argues against my own conclusion.**

**⇒ THE EXACT UNBLOCK CONDITION, so this does not become permanent.** One
editor-gated task, in one session, in this order:
1. run `Siege.Llama.SpikePrompt` and record the final `zoneA_tok` + Zone-B worst
   case;
2. run the generation-2 holdout via `Siege.Llama.SpikeEval holdout=<path>` **if
   it is ever going to be opened at all** — after this deletion it cannot be;
3. re-measure the partial-tier hitch if §6's gate item is still wanted;
4. apply 9b in the game lane;
5. **delete `SiegeLlamaSpike.cpp` and the two-model warning block in
   `USiegeLlamaSubsystem::Initialize()` in the same commit.**

⚠️ **`SpikeCorpus.inl` DOES NOT EXIST.** `find` over the whole repo returns
nothing. The board's `names` line has named a nonexistent file since TASK-423.
The spike loads its corpus from a CSV path at runtime (`LoadCorpus(Path, ...)`).
**Nothing else in `Source/` or `Plugins/` references any spike symbol** — only
comments — so the deletion, whenever it happens, is link-safe.

**MITIGATION SHIPPED IN THE MEANTIME:** `Initialize()` logs a **Warning naming
both command families** and telling the operator not to drive them in one
session. I considered a process-wide model lease enforcing mutual exclusion and
**rejected it**: my subsystem loads eagerly at `Initialize()`, so it would always
win the lease and **permanently lock out the spike** — destroying the exact
capability the deferral exists to preserve. A mitigation that defeats the reason
for the deferral is worse than the warning.

---

## 8. ⛔ SPEC ITEM (9): WHAT LANDED, WHAT DID NOT, AND WHY

| item | status |
|---|---|
| **(9) `MaxSnapshotTokens` = 400 as a real named constant** | ✅ **LANDED** in the plugin, where the enforcement is |
| **(9) `SnapshotPreFilterMaxChars` = 3000, role-derived** | ✅ **LANDED**, with the role declared beside the value |
| **(9) the re-anchored observable-truncation condition on BOTH new cut sites** | ✅ **LANDED** — both log; both reject rather than truncate (§5) |
| **(9a) `MaxUtteranceChars` bounds BOTH the `order:` and `pending:` lines** | ✅ **STRUCTURALLY SATISFIED, AND STATE WHY.** My enforcement is on the **whole snapshot region in tokens** — everything after the static prefix — so it counts both unbounded lines *and* the roster, together, without needing to know they exist. WARN-1's failure mode (a long typed sentence **and** a long pending line) cannot slip past a whole-region token count the way it slipped past a roster-width analysis. ⚠️ **It does not FIX 9a's underlying asymmetry** (the game lane still truncates only the roster) — it bounds the total. |
| **(9b) set `ZoneBCharReserve` from Zone B's PRINTED worst case** | ⛔ **NOT DONE — DOUBLY BLOCKED, DECLARED** |
| **RETIRE `MaxSnapshotChars` so a grep returns nothing** | ⛔ **NOT DONE — GAME LANE, DECLARED** |

**Why the last two could not be done here, verified by grep rather than assumed:**

```
Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h:266  static constexpr int32 MaxSnapshotChars = 1085;
Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.h:274  static constexpr int32 ZoneBCharReserve = 192;
```
plus **7 more uses** in `SiegeAssistantSnapshot.cpp` and **2** in
`SiegeAssistantComponent.cpp` — **all in the game module.**

1. **This task is scoped to the plugin** (*"Stay inside it"*), and that scope is
   exactly what makes it parallel-safe against the eight game-module tasks and
   TASK-447's compile gate. Retiring `MaxSnapshotChars` means editing
   `SiegeAssistantSnapshot.{h,cpp}` — **currently dirty in the working tree from
   another task** — and `SiegeAssistantComponent.cpp`, which is another owner's
   brand-new file. **That is precisely the concurrent-edit collision the quiet-
   module law exists to prevent.**
2. **9b additionally needs a measurement I am forbidden to take.** Its own
   wording is *"set the reserve FROM Zone B's **printed** worst case … re-measure
   it, do not eyeball it."* No editor, no PIE. ⛔ **I will not substitute a
   derived number for a printed one** — that is the exact defect class §8's
   TOKEN PROVENANCE spent a whole section retiring.

**⇒ MANAGER: these need ONE game-lane task, and it is small.** Retire
`MaxSnapshotChars` at all 10 sites, and set `ZoneBCharReserve` from a printed
reading. ✅ **The reading no longer needs the spike:**
`USiegeAssistantComponent::ReportFirstCapture` prints `zoneB_chars` from the
**shipped** builder on a live board — better evidence than the spike's 13-kind
fixture, which is what the current 192 was argued against (68 t0 / 71 t1).

---

## 9. DECLARED ADDITIONS TO THE PLUGIN'S SURFACE — RULE ON THESE

**§9 pins four methods + one delegate character-for-character; all five are
untouched.** These are *additions*, each with its reason. ⚠️ **None includes a
game-lane header and none names a Siegebound type** — grep confirms every
occurrence of "Siegebound" / "SiegeAssistant" in both files is inside a comment.

1. **`bool SetStaticPrefix(const FString&)` — THE ONE THAT NEEDS A RULING.**
   §8 states **both** *"the entire API surface between the two lanes is
   `(prompt, gbnf) -> string`"* **and** that the budget assertion is **mandatory**
   with `ZoneA_tokens` **tokenized from the assembled Zone-A string, never a
   recorded constant**. **These cannot both hold as written**: the plugin cannot
   build Zone A (it is `USiegeAssistantSnapshot::BuildZoneA`, behind the very
   boundary the first clause draws), so under the first clause alone the second
   is unimplementable except by the baked constant it forbids by name.
   **Resolution taken:** the Zone-A *string* crosses as an opaque `FString`,
   exactly like `Prompt`. **It is optional** (the subsystem runs without it and
   says loudly what is unchecked), and **it is checked, not trusted** — every
   request verifies `Prompt.StartsWith(Prefix, ESearchCase::CaseSensitive)` and
   logs a Warning if it does not, which also catches Zone A losing byte-stability.
   ⚠️ **If QA rules this out, §8's mandatory assertion has no implementable form
   and the manager must re-open the clause — please say which, rather than
   deleting the method.**
2. **`FSiegeLlamaSoftTimeoutSignature OnRequestSoftTimeout`** (parameterless
   multicast). §10 specifies the soft timeout as *"tells the FSM to say
   something"*; a constant the FSM has to poll is not a signal. It broadcasts on
   the **game thread at the moment 4 s is crossed** — checked in **both** the
   prefill and decode loops, because on the cpu tier the prefill alone can
   outlast 4 s and a soft timeout that only watches the decode is silent for
   exactly the slowest turns. It is dropped if the request already finished.
   ⚠️ **My first draft carried it as a flag on the completion — i.e. it arrived
   with the answer, after the wait it was meant to cover. Caught on re-read.**
3. **`HandleWorkerCompletion` / `HandleSoftTimeout`** — public only because the
   completion task must reach them through a `TWeakObjectPtr`. Not API.
4. **`GetOffloadTier()` / `DescribeState()`** — read-only, for logs and
   `Siege.Llama.Status`.
5. **Three `#if !UE_BUILD_SHIPPING` console commands** — `Siege.Llama.Status`,
   `Siege.Llama.Test`, `Siege.Llama.Cancel`. §5-legal (`FAutoConsoleCommand`, new
   file, `Siege.Llama.*` namespace, never a `UFUNCTION(exec)` on a gameplay
   class). **Without them TASK-447's gate is unrunnable** (§6).

---

## 10. What QA should scrutinise

1. **The thread-ownership map in the header comment — check it by READING, which
   is what it was written for.** Trace every path that could touch `Context` off
   the worker. I claim there are none; the two callbacks (`AbortThunk`,
   `LoadProgressThunk`) touch only atomics and are the only things running on
   foreign threads.
2. **`RequestCompletion` returns `false` on FOUR paths and OnComplete fires on
   NONE of them** (not ready · busy · empty prompt · pre-filter). Confirm the
   contract reads unambiguously: *false ⇒ nothing started, no callback ever.*
3. **The `bWorkPending` / `State==Busy` split** (§2). This is where a re-entrancy
   bug lived in my first draft. Convince yourself the worker cannot run a request
   twice.
4. **`ClearBusy()` happens BEFORE the delegate fires** — deliberate, so a handler
   issuing the next turn succeeds. Rule on whether that is the wanted semantics.
5. **⛔ Rule explicitly on REJECT-vs-TRUNCATE at both cut sites (§5).** I read a
   clause written about truncation and chose not to truncate. Reasoned, but it is
   an interpretation.
6. **⛔ Rule explicitly on `SetStaticPrefix` (§9.1).** It is the only real
   widening of the lane surface and it exists to make a mandatory clause
   implementable.
7. **The synchronous `EnsureBackendsLoaded()` on the game thread (§6).** I judged
   a data race on module state worse than a bounded, logged, one-time cost. Rule
   on it; it is the weakest point in the "never blocks match start" claim.
8. **The tier thresholds against `handoffs/TASK-413-buildmaster.md`.** Every
   constant should trace to a printed figure. `VramGameReserveMiB = 768` is the
   one I derived (from the measured 617.4 MiB survivable floor) — check that
   derivation.
9. **The header citations.** I corrected two of my own (`llama.h:323-325`,
   `llama.h:971-975`). Please re-check the rest against the vendored header
   rather than against this handoff.
10. **§13 does not bind this deliverable** — I added no test file and no
    `TestEqual`. `TASK-451-programmer.md` already exists in the tree, so the sweep
    has run; my files add nothing to its scope.
11. ⚠️ **`Grep` mangles comment syntax on this machine (§10 tooling trap).** Both
    files are comment-dense. **Do any comment-level or structural finding on raw
    `Read` output.** I verified ASCII-cleanliness and the absence of a literal
    comment terminator inside `//` comments with targeted greps, and both came
    back clean — but a *finding* about my comments should come from `Read`.

---

# 11. ⛔ APPENDED 2026-08-03 — QA LOOP 1 of 3: THE `C2555` FIX. **APPENDED, NOT OVERWRITTEN; NOTHING ABOVE ALTERED.**

**M8 DECLARATION DUTY, verbatim:** *adds no replicated property, no new replicated class, no new
relevancy tier.*

⛔ **I did not compile, did not open the editor, did not touch MCP/PIE, did not touch Git, did not
touch `Content/`, did not open the header, and did not touch 🔒 `SiegeLlamaSpike.cpp` (§16).**

## 11.1 THE DIAGNOSTIC AND THE THREE-PART FIX

`SiegeLlamaSubsystem.cpp(285,15) error C2555` — `FSiegeLlamaWorker::Run` was declared
`virtual void Run() override;` against `FRunnable`'s `virtual uint32 Run() = 0;`
(`HAL/Runnable.h:45`). Not covariant, so the override is ill-formed.

| # | site (by symbol, §18c) | change |
|---|---|---|
| 1 | `FSiegeLlamaWorker`, the `//~ FRunnable` block | `virtual void Run() override;` → `virtual uint32 Run() override;` |
| 2 | `FSiegeLlamaWorker::Run` definition | `void FSiegeLlamaWorker::Run()` → `uint32 FSiegeLlamaWorker::Run()` |
| 3 | the function tail, **after** `UnloadModel()` and `State.Set(Stopped)` | **+1 line: `return 0;`** |

**Behavioural lines: 3. Everything else added is comment.** Parens **938 → 955**; ⭐ **all +17 are
pairs inside comment prose, itemised: 10 in the declaration block, 7 in the return block — ZERO in
code.** Braces **215/215, unchanged** (a `return` adds none). **Zero non-ASCII bytes.** Every one of
the file's 39 `*/` is a block-comment end or the pre-existing `/*Progress*/` marker — **no comment
terminator inside any line comment I added** (§10's trap).

## 11.2 ⭐ THE TRACE — WHY "A RETURN ON EVERY EXIT PATH" TURNED OUT TO BE **ONE** RETURN

**This is the §20 work, and the answer is not the one the error message implies.** Build-master was
right to refuse to type it, and right that the loop is the ratified one — **but the traced result is
that the conversion is safe for a reason that had to be established rather than assumed.**

**`FSiegeLlamaWorker::Run()` HAS EXACTLY ONE EXIT.** Enumerated by reading the whole body, then
confirmed mechanically over the function's own line range:

| candidate exit | what it actually does |
|---|---|
| `return` statements in the body | ⭐ **ZERO. There are none.** |
| the `break` on `bStopRequested` | ⛔ **leaves the WHILE LOOP, not the function.** It jumps to `UnloadModel()` |
| the `while (!bStopRequested)` condition going false | falls to the same `UnloadModel()` |
| an SEH fault in `LoadModelGuarded` / `RunRequestGuarded` | ⛔ **cannot propagate.** `SehInvoke`'s `__except (EXCEPTION_EXECUTE_HANDLER)` swallows it and **returns `false` normally**; control resumes in the loop |

⇒ **Both loop exits CONVERGE on one tail — `UnloadModel();` then `State.Set(Stopped);` — and control
then falls off the end of the function.** The single `return 0;` goes **there**, at the point control
already left.

⇒ ⭐ **THEREFORE THE CONVERSION INTRODUCED NO NEW EXIT PATH AT ALL.** §20's tell — *"a proposed fix
that introduces a NEW way to leave a loop must be checked against every guard that runs AFTER it"* —
**is satisfied vacuously here, and that is a finding, not a dodge:** the dangerous version of this
fix is the one that never got typed.

### ⛔ THE TRAP I DID NOT TAKE, RECORDED BECAUSE IT IS THE OBVIOUS ONE

**Converting the `break` into `return 0;` looks equivalent and is not.** It would skip
`UnloadModel()` **and** `State.Set(EState::Stopped)`, which would:

1. **leak the ~2.5 GB model**, and
2. let `JoinAndDestroy()`'s `WaitForCompletion()` hand the game thread a joined worker whose state
   still reads `Loading`/`Idle`/`Busy` — i.e. `Deinitialize()` would complete while the object lied
   about being stopped.

**That is exactly the shape §20 warns about**, and it is now written into the code as an anti-revert
comment so the next editor does not "simplify" it.

### ✅ WHAT IS PROVABLY UNDISTURBED — THE DO-NOT-DISTURB LIST, CHECKED AT THE ARTIFACT

⛔ **I changed nothing inside the loop body.** The dispatch branch, its `else` branch and its
`PostCompletion(false, …)` are byte-identical and still run to completion before control reaches the
new line.

| ratified item | owner | verified |
|---|---|---|
| `MarkBusy()` refuses unless `Idle`; `MarkBusy()` **before** `bWorkPending`; run-loop state re-test + else-branch completion | TASK-457 | ✅ **untouched** |
| deadline test at the **BOTTOM** of the prefill loop body (a top-of-body test is inert — §21) | TASK-457 | ✅ **untouched** |
| `bAborted` (not `bCancelled`) + the extended post-prefill guard on all three flags | TASK-457 | ✅ **untouched** |
| **both** elapsed args based on `StartSeconds` — prefill `FPlatformTime::Seconds() - StartSeconds`, decode `Now - StartSeconds` | TASK-459 | ✅ **untouched.** Neither was re-pointed at `PrefillStart`/`DecodeStart` |
| `HardTimeoutSeconds` **verbatim `10.0`** | fence | ✅ `SiegeLlamaSubsystem.h:161` |
| no header claim softened | fence | ✅ **the header was never opened** — mtime still **13:25** (TASK-457's); the `.cpp` is 14:34 |
| 🔒 `SiegeLlamaSpike.cpp` (§16) | fence | ✅ **UNMODIFIED** — it **is tracked**, so `git diff --stat` is the valid instrument here and it returned **empty**; 188,736 B, matching TASK-447's pre-flight reading |

## 11.3 📌 §22 SWEEP — **IT FOUND ONE MORE THING AND THEN GENUINELY BOTTOMED OUT. THAT IS THE RESULT.**

**I swept for the SHAPE — "an `override` whose signature does not match its engine base" — not for
the line the compiler named.**

**(a) All four `FRunnable` overrides, re-read against `HAL/Runnable.h:32-61` itself:**

| override | base declares | ours | verdict |
|---|---|---|---|
| `Init()` | `virtual bool Init()` | `virtual bool Init() override { return true; }` | ✅ **exact** |
| `Run()` | `virtual uint32 Run() = 0` | was `void` | ⛔ **THE DEFECT — fixed** |
| `Stop()` | `virtual void Stop()` | `virtual void Stop() override {...}` | ✅ **exact** |
| `Exit()` | `virtual void Exit()` | `virtual void Exit() override {}` | ✅ **exact** |
| destructor | `virtual ~FRunnable() = default` | `virtual ~FSiegeLlamaWorker() override` | ✅ **correct** |

⇒ ⭐ **`Run` was the only divergence of the four. Three siblings were already right.**

**(b) `GetSingleThreadInterface()` — swept, NOT a defect, and I am recording the verdict rather than
the silence.** It is the one `FRunnable` virtual we do **not** override, so the base returns
`nullptr` ⇒ this runnable is **not ticked when `FPlatformProcess::SupportsMultithreading()` is
false.** ✅ **That is correct for this class**: a multi-second CPU burn must never be pumped on the
game thread, so on such a platform the assistant is simply unavailable — the same degradation as a
missing model file, which the design already handles as one log line. **Written into the comment so
nobody "completes the interface" later.**

**(c) The two overrides in the PUBLIC header — and this one is better than a re-read:**
`Initialize(FSubsystemCollectionBase&)` / `Deinitialize()` match `Subsystem.h:59` / `:62` exactly.
⭐ **They were ALREADY COMPILE-PROVEN by the failing build itself** — 16 game-lane TUs `#include`
this header and produced zero diagnostics, and a bad `override` in a header is an error in *every*
TU that parses the class. ⇒ ⚖️ **THE REAL LESSON, AND IT EXPLAINS WHY *THIS* CLASS WAS THE ONE THAT
ROTTED: `FSiegeLlamaWorker` is private to the `.cpp`, so it has EXACTLY ONE READER IN THE ENTIRE
BUILD.** The header's overrides get re-validated 16 times per build; the worker's get validated once.
**A private implementation class gets one chance, and that is where the signature drift landed.**

**(d) Repo-wide: `FSiegeLlamaWorker` is THE ONLY `FRunnable` SUBCLASS IN THE WHOLE REPOSITORY.** The
only other `FRunnable` mentions are comments — including 🔒 `SiegeLlamaSpike.cpp`, which states in
terms that it uses `AsyncThread` and is **not** an `FRunnable` class. ⇒ ✅ **The shape cannot recur
elsewhere. The sweep is closed, and it is closed by a positive enumeration rather than by a bare
negative (§14).**

## 11.4 ⚠️ WHAT REMAINS UNVERIFIABLE WITHOUT A COMPILE — STATED, NOT OMITTED

1. ⛔ **THAT IT COMPILES.** I did not build. The mechanical checks above are necessary, not
   sufficient. **`return 0;` after the loop means every path returns**, so MSVC C4715
   (*"not all control paths return a value"*) should not fire — but **that is a prediction.**
2. ⛔ **THIS IS NOT NECESSARILY THE COMPLETE ERROR SET FOR THIS TU.** A failing translation unit
   stops at its own errors; the compiler never got past `:285` to parse the rest of the class or the
   ~2,000 lines below it. ⛔ **"Fix this one and the file builds" is NOT a claim I am making.**
3. ⛔ **THE LINK STEP HAS STILL NEVER RUN** (TASK-447 stopped at 18 of 23 actions). **Unresolved
   externals remain unproven in EITHER direction.** ⚠️ **`Run()` is dispatched by the engine through
   `FRunnableThread::Create`, so the return-type change is an ABI-visible change to a virtual slot —
   harmless because the class has one TU and no other implementer, but that is reasoning, not a
   linked binary.**
4. ⛔ **NOTHING EXECUTED.** No smoke test, no `Siege.Llama.*` command, no deadline exercise, no
   `EnsureBackendsLoaded()` timing, **no Zone B reading.** Every caveat in §6 above still stands
   exactly as written, and **§21 applies: absence of a log line is not evidence a guard works.**

✅ **ONE CAVEAT CLASS IS DISCHARGED, AND I AM RECORDING IT BECAUSE IT WAS MINE:** **UHT accepted the
entire batch** (`Module.SiegeLlama.cpp` compiled, action 9 of 23) ⇒ **every UHT-related caveat in
this handoff is discharged.** The plugin's reflection surface is valid.

## 11.5 WHAT QA SHOULD SCRUTINISE

1. ⭐ **THE ONE-EXIT CLAIM IS THE WHOLE ARGUMENT — CHECK IT, DO NOT TAKE IT.** Read `Run()` end to
   end and satisfy yourself there is **no** `return` other than the new tail one, and that the
   `break` is loop-scoped. **If a second exit exists, my "no new early exit" reasoning collapses and
   the guards below it must be re-examined.**
2. ⭐ **CONFIRM `return 0;` IS AFTER `UnloadModel()` AND `State.Set(Stopped)`, NOT BEFORE EITHER.**
   That ordering is the entire safety property, and the two statements are silent about themselves.
3. **Confirm the declaration and the definition agree** (`uint32` at both). A mismatch is a fresh
   error, not the old one. The only surviving `void Run()` text is **inside a comment** recording the
   history — verify it is a comment, not a stray declaration.
4. **Rule on whether the return VALUE should be `0`.** Nothing in this plugin reads
   `FRunnable::Run`'s exit code, and faults are reported through `EState::Faulted` + the completion
   delegate. **I judged a non-zero code to be a claim no caller could act on.** If you want the
   faulted session reflected in the thread exit code, say so — it is a one-line change with no
   current reader.
5. **§22's sweep result (11.3) is a claim about the whole repo.** The load-bearing part is that
   `FSiegeLlamaWorker` is the only `FRunnable` subclass; if that is wrong, other classes may carry
   the same drift.
6. ⛔ **Do not re-litigate the do-not-disturb list.** TASK-457's and TASK-459's work is byte-intact
   (table in 11.2) and neither task is in a loop.
