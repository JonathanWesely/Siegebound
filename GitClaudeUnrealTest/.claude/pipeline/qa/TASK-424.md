# QA Report — TASK-424 (gate on TASK-423 **and** TASK-450)

**Reviewer:** qa-reviewer · **Date:** 2026-08-03 · **Mode:** pre-compile source review, read-only, no edits

## VERDICT — PER TASK

| task | deliverable | verdict | blockers |
|---|---|---|---|
| **TASK-423** | `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeAssistantZoneATest.cpp` | ✅ **PASS** | **0** |
| **TASK-450** | `Plugins/SiegeLlama/Source/SiegeLlama/{Public,Private}/SiegeLlamaSubsystem.{h,cpp}` | ✅ **PASS** | **0** |

**Combined: PASS · 0 BLOCKERS · 8 WARN · 7 NIT.** Both are cleared to enter TASK-447's compile gate.

---

## ⛔ THE THREE THINGS THAT MUST NOT DRIFT — POLICED FIRST, RESTATED HERE

1. ⛔ **"THE TESTS PASS" IS BARRED AND NOTHING IN THIS REPORT CLAIMS IT.** Nothing in this batch has been
   built. **No test in this repository has ever executed.** Every statement below is a claim about **source
   text I read with `Read`**, and nothing more. Discharge is TASK-447's compile **plus a green run**, claimed
   against **that run**.
2. ⚠️ **WARN-5 IS INSTRUMENTED, NOT DISCHARGED — AND THIS GATE DOES NOT MOVE IT.** `Siegebound.Assistant.ZoneA.*`
   has never run. A PASS here says the instrument is **correctly built**, not that the property it measures
   holds. ⛔ **Nobody may cite `qa/TASK-424.md` as WARN-5's discharge.**
3. ⛔ **BAR #5 STANDS UNCLEARED AT 20/25 AGAINST 22, WITH ZERO REFUSE-CLASS FAILURES STILL REQUIRED.**
   **Nothing in either task is accuracy progress.** TASK-423 is an equality instrument; TASK-450 is plumbing.
   Neither touches a prompt, a grammar, a synonym or a corpus row.

---

## THE FOUR RULINGS TASK-450 ASKED FOR BY NAME

### ⚖️ RULING 1 — `SetStaticPrefix`: **RATIFIED.** And the programmer is right — §8's two clauses cannot both hold as written.

**I verified the contradiction independently rather than accepting the claim.**

- CONVENTIONS §8 ZONE-A SIZE BOUND ruling 2 (`CONVENTIONS.md:753-762`) makes the startup budget assertion
  **MANDATORY** — *"A subsystem that ships without this assertion has not implemented this clause"* — and
  `:757` forbids the alternative **by name**: *"`ZoneA_tokens` IS TOKENIZED AT LOAD FROM THE ASSEMBLED ZONE-A
  STRING — NEVER THE LITERAL `1139`, NEVER ANY RECORDED FIGURE."*
- The assembled Zone-A string is produced by `USiegeAssistantSnapshot::BuildZoneA`
  (`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantSnapshot.cpp:768`) — **in the game module.**
- §8's lane split (and `TASKBOARD.md:5099-5100`) forbids the plugin from including any game-lane header.

⇒ Under a literal reading of *"the **entire** API surface between the two lanes is `(prompt, gbnf) -> string`"*,
the plugin can obtain `ZoneA_tokens` only by (i) a baked constant — **forbidden by name**, or (ii) not
asserting — **forbidden by name**. **The two clauses are genuinely unsatisfiable together as written.**
**The programmer's reading is correct and this is the fourth spec line correctly refused this wave (§15).**

**WHY THE IMPLEMENTATION IS RATIFIED RATHER THAN MERELY TOLERATED.** The clause's own stated purpose —
restated identically in §8, in `TASKBOARD.md:5099`, and in the header's invariant 1
(`SiegeLlamaSubsystem.h:65-68`) — is *"no game-lane include, no Siegebound type in any signature."*
`bool SetStaticPrefix(const FString&)` (`SiegeLlamaSubsystem.h:286`) adds **neither**. It carries an opaque
`FString`, exactly as `Prompt` and `Grammar` already do.

**I CHECKED THE LANE SPLIT AT THE ARTIFACT RATHER THAN TAKING THE HANDOFF'S WORD.** Every occurrence of
`Siegebound` / `SiegeAssistant` in both new files is **inside a comment**: `SiegeLlamaSubsystem.h:66, :265,
:277` and `SiegeLlamaSubsystem.cpp:1002, :1008, :1015`. Zero includes of any game-lane header
(`SiegeLlamaSubsystem.cpp:1-33`). **§8's lane split is INTACT.** The plugin still cannot name a kind, a place,
an intent or a snapshot.

**Three further properties that make this the smallest resolution rather than a convenient one:**
- **It is OPTIONAL**, and the cost of its absence is stated **out loud, once per session, at Warning**
  (`:1237-1240`) rather than left to look like a pass. That is §8's *"a guardrail that cannot observe the
  thing it guards is worse than none, because it reports safe"* obeyed, not evaded.
- **It is CHECKED, NOT TRUSTED** — every request verifies `Prompt.StartsWith(Prefix, ESearchCase::CaseSensitive)`
  (`:1224`), case-**sensitively**, which also catches Zone A losing byte-stability.
- **The assertion's operands are live**: `ZoneA_tokens` from `llama_tokenize` at the instant of registration
  (`:1050, :1058-1059, :1077-1079`), `n_ctx_actual` from `llama_n_ctx(Context)` (`:864`) and never the
  requested 2048. **`1139` appears nowhere as an operand** — only in a header comment explaining the sanity
  arithmetic (`SiegeLlamaSubsystem.h:199`). §8's "eighth seam in a second costume" is avoided.

⛔ **DO NOT DELETE THE METHOD.** ⇒ **See SPEC-1 below: the manager owes §8 a one-line amendment.**

### ⚖️ RULING 2 — REJECT-rather-than-TRUNCATE at both cut sites: **RATIFIED, and it is strictly stronger than the clause asks.**

§8's re-anchored condition (`CONVENTIONS.md:720-721`) binds **the act**: *"IF THE SNAPSHOT REACHES THE MODEL
SHORTER THAN IT WAS BUILT, SOMETHING SAYS SO"*, with the QA hook *"a subsystem that TRUNCATES the snapshot
without an observable log has not implemented this clause."*

**Rejecting means the antecedent is never satisfied — nothing ever reaches the model shorter than it was
built, because nothing reaches it at all.** And both cuts are observable anyway:
- **AUTHORITY** (`MaxSnapshotTokens = 400`, tokenised) — `SiegeLlamaSubsystem.cpp:1244-1258`, Warning naming
  both counts, then `PostCompletion(false, …)`.
- **PRE-FILTER** (`SnapshotPreFilterMaxChars = 3000`, chars, game thread) — `:1791-1802`, Warning naming both
  counts, then `return false`.

**The domain argument is checkable and it holds.** §8 (`CONVENTIONS.md:687`) defines Zone C as *"roster,
pending-intent line, **the utterance**"* — so the snapshot's tail **is** the player's own sentence. A tail
truncation would leave a syntactically perfect prompt about nothing, and §1's central law is precisely that
this assistant must never emit a confident wrong order. **REJECT is the correct reading.**

⇒ **Also ruled: §8 resolution (2)'s demand is met** — the authority is enforced **by tokenizing**
(`:1244`), not by a char cap, and `SnapshotPreFilterMaxChars = 3000` is sized from the floor
(`400 × 3.56 = 1424`) with the **role declared in the same breath as the value**
(`SiegeLlamaSubsystem.h:151-170`). **1085 is carried into nothing.** ✅ *"Carrying 1085 forward under any name
is a QA FAIL"* — it was not carried.

⚠️ **One consequence the handoff does not state — see WARN-6.**

### ⚖️ RULING 3 — Synchronous `EnsureBackendsLoaded()` on the game thread: **ACCEPTED, WITH TWO BINDING CONDITIONS ON TASK-447.**

**The stated mechanism is checkable and I checked it:** `FSiegeLlamaModule::bBackendsLoaded` is a **plain
`bool`** (`SiegeLlamaModule.h:83`, no atomic, no lock), and `Siege.Llama.Info` calls
`Module->EnsureBackendsLoaded()` from the console (`SiegeLlamaInfo.cpp:81`). ⇒ **Calling it from the worker
would be a genuine data race on module state this file does not own.** The refusal is not preference; it
cites an artifact (§15's ratifiability test).

**What bounds the cost, verified in order at `SiegeLlamaSubsystem.cpp:1624-1700`:**
- it runs **only after** the model file is confirmed present (`:1649`) — a machine with no weights never pays it;
- it is the **cheap half** (DLL registration + `llama_backend_init`, `SiegeLlamaModule.cpp:192-194`) — the
  **2.5 GB weight read stays on the worker** (`:1700` → `Run()` → `LoadModelGuarded`);
- it is **timed** (`:1663, :1665`) and **the number is logged** (`:1675-1677`).

⛔ **CONDITIONS, BOTH BINDING ON TASK-447:**
1. **TASK-447 MUST REPORT THE MEASURED `LLM_LOAD: ggml backends ready in N ms` FIGURE.** The gate item
   *"async load never blocks match start"* may be recorded **only as QUALIFIED-PASS with N beside it** —
   never as a bare PASS. A claim that something does not block is not verifiable without the number.
2. **A large N is a finding for Jonathan, not a footnote.** Nobody has ever measured it.

### 🔒 RULING 4 — The spike deletion, deferred a SECOND time: **CORRECT, RATIFIED — AND TASK-424's OWN SPEC ITEM (10) IS SUPERSEDED AND MAY NOT BE ENFORCED.**

**I VERIFIED THE DEPENDENCY MYSELF, AT FOUR ARTIFACTS, RATHER THAN RELAYING THE HANDOFF:**

| claim | verified at |
|---|---|
| the gen-2 sealed holdout **exists** | `Docs/Data/assistant_eval_holdout2.csv` — present on disk |
| it is authored **against the `t0` fixture** | its own rows say so by name: `HOLD2-01` *"3 longbowmen are alive **at t0**"*, `HOLD2-02` *"(5 alive)"* |
| `t0` lives **inside the spike** | `SiegeLlamaSpike.cpp:509-519` — *"t0 — THE SEALED CORPUS'S BOARD. ⚠️ ITS BYTES ARE FROZEN … THE EVAL JOB MUST ALWAYS USE t0"* |
| the **only scorer** lives inside the spike | `Siege.Llama.SpikeEval` registered at `SiegeLlamaSpike.cpp:4744-4751`; `Siege.Llama.SpikePrompt` at `:4753-4759`. **Neither name exists anywhere else in `Plugins/` or `Source/`.** |

⇒ **DELETING `SiegeLlamaSpike.cpp` WOULD MAKE THE SEALED GEN-2 CORPUS UNMEASURABLE — destroying Jonathan's
one unspent measurement inside a commit whose stated purpose is tidiness. The deferral is CORRECT.**

⛔ **AND IT IS NO LONGER A DEFERRAL AT ALL.** **CONVENTIONS "Settings screen…" §16 (`CONVENTIONS.md:1362-1377`)
has already RULED this**: `SiegeLlamaSpike.cpp` is **reclassified from THROWAWAY to LOAD-BEARING TEST
INFRASTRUCTURE**; the deletion is **UNSCHEDULED, not deferred**; it *"may not be re-scheduled until a named
replacement exists for EVERY capability"* (four, enumerated at `:1368-1372`); and it *"is deleted only by a
task whose SOLE deliverable is migrating all four capabilities — never as a step inside a task that is doing
something else."* §16 also discharges Wave-0 ruling 4 **on its own terms** (`:1376`): the spike is dev/test
console surface, not a shipping path — *"what ruling 4 got wrong was not the danger; it was the noun."*

⇒ ⛔ **TASK-424 SPEC ITEM (10) — *"The spike harness is DELETED … two load paths coexisting is a BLOCKER"* —
IS SUPERSEDED BY §16 AND I REFUSE TO ENFORCE IT.** Enforcing it would spend a sealed artifact.
**This is a finding about the board, not about the code.** ⇒ **BOARD-2.**

**The mitigation is ADEQUATE and the rejected alternative was rejected for the right reason.**
`Initialize()` logs a Warning naming both command families (`SiegeLlamaSubsystem.cpp:1696-1698`). The
process-wide model lease was considered and rejected because the subsystem loads eagerly and would **always
win the lease, permanently locking out the spike** — i.e. the mitigation would have destroyed the exact
capability the deferral exists to protect. **That reasoning is sound and is ratified.**

⚠️ **`SpikeCorpus.inl` DOES NOT EXIST — CONFIRMED BY ME, ON THE FILESYSTEM, NOT BY SEARCH.** A `Glob` for
`**/SpikeCorpus.inl` over the whole repository returns nothing; the spike loads its corpus **by path at
runtime**. §16 (`:1377`) already strikes it from all `names:` lists; **the board has not been updated.**
⇒ **BOARD-3.**

---

## VERIFIED AT THE ARTIFACT — the checks the dispatch named

### ✅ The two live llama symbols — CONFIRMED BY ME AGAINST THE VENDORED HEADER (§14)

I did **not** rely on the programmer's grep. `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/include/llama.h`,
read directly:

| symbol | status |
|---|---|
| `LLAMA_LOAD_MODE_MMAP` | ✅ **`llama.h:207`** — exact. Member `load_mode` present at **`llama.h:315`**. Used at `SiegeLlamaSubsystem.cpp:797`. |
| `llama_memory_seq_rm` | ✅ **`llama.h:735`** — exact. Doc block `:730-739` matches the code's citation. Used at `:1306, :1316`. |
| `use_mmap` | ⛔ **absent.** I read the **whole** `llama_model_params` struct (`llama.h:306-341`) — there is no such member. |
| `llama_kv_cache_seq_rm` | ⛔ **absent** — 0 hits across the whole `include/` tree. |

**§14's negative-search law is honoured:** the two positives are confirmed by **reading the declarations**;
the two negatives are confirmed by **reading the containing struct in full**, not by a bare negative. Both
dead spellings appear in the new file only inside comments that say they are dead (`:791-796, :1303-1305`).

**Every other vendored citation in the new file re-checked and EXACT:** `llama.h:307` (`devices` — *"if NULL,
all available devices are used"*) · `:323-325` (progress callback) · `:382-385` (abort callback, **verbatim**,
including *"currently works only with CPU execution"*) · `:551-556` (query the actuals) · `:971-975`
(`llama_decode` return codes `0/1/2/-1/<-1`). ⚖️ **This matches §8's own prior: `llama.h` anchors hold, game-lane
anchors rot.** The two game-lane anchors the handoff cites were also checked and **both land exactly**:
`SiegeAssistantSnapshot.h:266` = `MaxSnapshotChars = 1085`, `:274` = `ZoneBCharReserve = 192`.

**No deprecated UE or llama API is used.** `llama_n_layer` / `llama_n_ctx_train` / `llama_token_is_eog` /
`llama_new_context_with_model` are all `DEPRECATED` at `llama.h:561-566, :532, :1119` — **the code uses the
modern `llama_model_n_layer`, `llama_vocab_is_eog`, `llama_init_from_model` throughout.** ✅

### ✅ Tiering thresholds — PROVENANCE traced to TASK-413's **printed** figures, not just the arithmetic

Checked against `handoffs/TASK-413-buildmaster.md`:

| constant | value | printed source |
|---|---|---|
| `FullOffloadCostMiB` | 2703 | `vram_delta=2702.8` (`:638`) and `2701.8` (`:1089`) — **two readings 1 MiB apart** |
| `PartialOffloadCostMiB` | 1264 | `793.5` (`:644`) **and** `1264.0` (`:1090`) — ✅ **the WORSE of the two is used, per §12h**, and the 1.6× spread travels with it in the code's own comment (`SiegeLlamaSubsystem.cpp:71-74`) |
| `VramGameReserveMiB` | 768 | derived from `vram_free_after = 617.4 MiB` (`:638`) — see **WARN-3** for the one imprecision in *how* it is cited |
| cpu-tier warning text | 2 of 5 aborted, truncated JSON, wall 8730 ms | `:1104`, `:961` — quoted **exactly** at `SiegeLlamaSubsystem.cpp:766-768` |

**Arithmetic checked, and the policy lands where the board says it should:** full threshold `2703+768 = 3471`;
TASK-413's machine had **3320.2 MiB free** ⇒ **below it** ⇒ partial threshold `1264+768 = 2032` ⇒ **PARTIAL**.
⇒ ✅ **The default tier is reached from the measurements rather than hard-coded to them.**

⚠️ **CONSEQUENCE FOR TASK-447, STATED SO THE GATE IS NOT MISREAD: on this machine the subsystem will select
`tier=partial`, not `full`.** Therefore `Siege.Llama.Test long cancelms=200` exercises the **ubatch-boundary**
cancel, **not** the mid-graph one — `abort_callback` is documented CPU-only (`llama.h:384`). **The run must
state its tier.** A cancel measured on a GPU tier is **not** evidence for the mid-prefill claim.

### ✅ The three budget guards — and the confirmation there is **no unguarded path to `llama_decode`**

**I traced every path into `llama_decode` (`:1355` prefill, `:1477` decode) back to its guards.** The only
entry point is `RunRequestUnguarded`, and **every** early exit precedes both `llama_decode` sites:

`:1178` no context/vocab → return · `:1186` tokenize failed → return · `:1245` authority cut (only when a
prefix is registered **and** matches) → return · **`:1265` THE CONTEXT BUDGET — `PromptTokens.Num() +
MaxOutputTokens > n_ctx_actual` → return, UNCONDITIONALLY, on every request.**

⇒ ✅ **CONFIRMED: the context budget is enforced on every single request, with no prefix required.
There is no unguarded path to `llama_decode`.** The handoff's claim holds.

⇒ ✅ **AND THE INERT STATE PRESENTS AS INERT, NOT AS A PASS.** `MaxSnapshotTokens` and
`SnapshotPreFilterMaxChars` are both gated on a registered prefix, and their absence logs at **Warning**,
once per session, naming **both** constants and stating what is not checked (`:1237-1240`); the incomplete
startup assertion does the same (`:937-941`). ⛔ **A clean log is NOT these budgets passing — TASK-447 must
read the Warning.** This is the honest shape and it is credited, not merely tolerated.

### ✅ TASK-423's spike lane — the 98-line copy is **GENUINELY VERBATIM**. I ran the diff.

**Method (raw `Read` on both, no grep — §14's rendering trap):** I extracted every `Out += TEXT(...)` line
from `SiegeLlamaSpike.cpp`'s `AppendZoneA` (`:220-354`, comments excluded) and compared it in order against
`SiegeAssistantZoneATest.cpp:105-202`.

**RESULT: 98 emitting lines on each side, in identical order, character-for-character identical.**
Spot-anchors, all matching: spike `:224` ↔ test `:105` · `:239` ↔ `:115` · `:284` ↔ `:148` (`- Choose a place
by its noun …`) · `:305` ↔ `:168` (the long `ancient_ground_near` alias row) · `:312` ↔ `:172` (the
`nearest_mine` row after the `gold` alias deletion) · `:327` ↔ `:186` · `:353` ↔ `:202`. The three
comment-only gaps in the spike (`:277-279`, `:309-311`, `:326`) correctly emit nothing.
⇒ ✅ **The programmer's "98 lines, verbatim match" is CONFIRMED independently.**

⚠️ **AND THE LIMIT TRAVELS WITH THE RESULT (§9c).** This is a **same-authorship** copy: it cannot detect an
error the two lanes **share**, and the offline sha256 agreement in the handoff §3 is a **prediction, not a
discharge** — the programmer says so first, which is exactly right. **The test is a strong result, not a
total one.** Once the spike is gone this block's status changes from *copy* to *evidence* and the file says
so in three places (`:79-88`); **keep it that way.**

### ✅ §13 comparator law — TASK-451's "ZoneA already compliant" **VERIFIED, NOT ASSUMED**

Every assertion in `SiegeAssistantZoneATest.cpp` enumerated and its claim-shape matched to its comparator:

| line | claim | comparator | verdict |
|---|---|---|---|
| `:363` | *"BYTE-IDENTICAL to the spike lane"* | `TestEqualSensitive` | ✅ |
| `:520` | *"byte-identical strings"* | `TestEqualSensitive` | ✅ |
| `:531` | *"emit the same Zone A"* | `TestEqualSensitive` | ✅ |
| `:544` | *"emit the same Zone A"* | `TestEqualSensitive` | ✅ |
| `:591` | *"byte-stable across calls"* | `TestEqualSensitive` | ✅ |
| `:597` | *"is NOT the lane …"* | `TestNotEqualSensitive` | ✅ |
| `:594` | *"prints the deterministic `none` block"* | `Contains(..., ESearchCase::CaseSensitive)` | ✅ |
| `:401 :403 :411 :413 :475 :477 :604` | counts | `TestEqual`, **all `int32` vs `int32`** | ✅ §13: integers unaffected |

⇒ **5 `TestEqualSensitive` + 1 `TestNotEqualSensitive`; ZERO string claims on an insensitive comparator.**
Both overloads confirmed present in the installed engine: `AutomationTest.h:2016` and `:2023`.
⚖️ **And the divergence reporter is genuinely byte-level too** — `FirstDifference` (`:256-267`) compares
`TCHAR` by `TCHAR`, so a casing-only drift is located, not swallowed.

### ✅ §12g — no derived token constant anywhere

The **only** occurrences of `1139` / `zoneA_tok` in `SiegeAssistantZoneATest.cpp` are at `:55` and `:219`, both
inside comments **explaining why the figure is absent**. The sole numeric constant is
`MeasuredZoneAChars = 5116` (`:231`) — a **character** count, exact and countable offline. ✅ *Two artifacts,
two quantities, neither doing the other's job.*

---

## WARNINGS (8) — none blocks the compile gate

- **[WARN-1] `SiegeLlamaSubsystem.cpp:349` (`MarkBusy`) + `:549` (run loop) — the session fault latch can be
  overwritten by a concurrent request, and the run loop never re-checks it.**
  `ClearBusy()` (`:339-346`) is carefully guarded — *"it never clears a Faulted or Stopped state"* — but its
  twin `MarkBusy()` sets `State` **unconditionally**. The reachable window: `ApplyPendingStaticPrefix()` →
  `RunBudgetAssertion()` → `LatchFaulted()` (`:955`) is the **one** worker transition out of `Idle`, and the
  game thread can be between `IsReady()` (`:1754`) and `MarkBusy()` (`:304`) when it fires. The run loop then
  compounds it: `:549` dispatches on `bWorkPending` **alone** and never re-tests the state, so a faulted worker
  would still execute the request. **Inert today** (no game-lane caller, and `SetStaticPrefix` is refused while
  busy), which is why it is a WARN. **Fix, two lines:** guard `MarkBusy()` with `if (GetState() ==
  EState::Idle)`, and gate `:552` on `GetState() == EState::Busy`, completing on the else branch.

- **[WARN-2] `SiegeLlamaSubsystem.cpp:1344` — the hard deadline is NOT a backstop during PREFILL on the GPU
  tiers, though the header says it is.** The between-slice check tests only `bCancelRequested ||
  bStopRequested`; it does **not** test `AbortDeadlineUsec`. The deadline reaches the prefill only through
  `AbortThunk`, which `llama.h:384` documents as **CPU-only**. ⇒ On `tier=partial`/`full` a long prefill has
  **no** 10 s enforcement at all until the decode loop's check at `:1494`. The header's claim (`:130-135`,
  *"on Partial and FullOffload it is a BACKSTOP"*) is therefore stronger than the code. **Measured prefill is
  ~1.7 s on partial, so this does not bite today** — but the *claim* is what QA is asked to police. **Fix, one
  line:** add `|| ShouldAbortNow()` (or the direct deadline test) to `:1344`.

- **[WARN-3] `SiegeLlamaSubsystem.cpp:83-87` — `VramGameReserveMiB`'s derivation cites a bar measured on a
  DIFFERENT TIER than the VRAM state it names. §8's SCOPE sub-law, instance 6's shape.** The comment reads
  *"vram_free_before = 3320.2 -> after = 617.4 MiB, and **bar #1 (frame time) still PASSED on p99 in exactly
  that state**"*. The 617.4 figure is the **full-offload** run (`TASK-413:638`); **bar #1 is scoped to the
  PARTIAL tier** in both of its result rows (`TASK-413:599`, `:1103`). **The conclusion survives** — the same
  handoff states *"the p99 attributable delta is **+2.0 to +4.6 ms on every tier**"* (`TASK-413:625`), and
  that reading was itself taken with the grammar dead (~93 output tokens/iteration, i.e. **more** work than
  ships — the error direction is SAFE). ⇒ **The number 768 is sound; the sentence borrows a partial-tier
  bar's authority for an all-tier measurement.** **Fix:** cite `TASK-413:625` (*"every tier"*) instead of
  *"bar #1 … in exactly that state"*.

- **[WARN-4] `SiegeLlamaSubsystem.cpp:665-676` — `BestDiscrete` is the FIRST discrete GPU, not the best one.**
  The condition is `Type == GPU && BestDiscrete == nullptr` — free VRAM is never compared. On a two-discrete-GPU
  machine the tier is decided from device 0's headroom regardless. Every device **is** logged with its
  free/total (`:658-663`), so it is diagnosable — but the identifier states a selection policy the code does not
  implement, and the log would look perfectly correct. **Inert on Jonathan's single-GPU laptop.** **Fix:** either
  rename to `FirstDiscrete` or compare `FreeBytes`.

- **[WARN-5] `SiegeLlamaSubsystem.cpp:704-712` and `:686-697` — an unknown/incoherent headroom reading PROMOTES
  to PARTIAL, and the comment calls that "conservative".** `FreeMiB == 0` is treated as *"a missing reading
  rather than a measurement"* — but 0 is also what a genuinely full card reports, and in that case PARTIAL is
  the **unsafe** direction. Among *offload* tiers PARTIAL is conservative; among **all** tiers **CpuOnly** is.
  Same shape in the iGPU branch (`:695`). **Blast radius is bounded**: the alloc fails, `llama_model_load_from_file`
  returns NULL, `LatchFaulted` fires (`:813`) and the match stays playable — so this is a WARN, not a blocker.
  **Fix:** demote an unknown reading to `CpuOnly`, or say *"conservative among offload tiers"*.

- **[WARN-6] The two cut sites present TWO DIFFERENT caller-visible failure shapes for one class of failure,
  and the handoff does not say so.** The **pre-filter** rejects with `RequestCompletion` returning **`false`**
  (`:1800` — *"nothing started, OnComplete never fires"*), while the **authority** cut returns **`true`** and
  then completes with `bSuccess == false` (`:1256`). Both are correct; they are simply **not the same contract**,
  and the Wave-1 FSM must handle both or a budget rejection will strand the console in "thinking". **Fix:** one
  line in the `RequestCompletion` doc comment (`SiegeLlamaSubsystem.h:233-243`) naming both shapes.

- **[WARN-7 · SPEC-1] CONVENTIONS §8's *"the ENTIRE API surface between the two lanes is `(prompt, gbnf) ->
  string`"* IS FALSE AS WRITTEN, given §8's own mandatory Zone-A assertion. THE MANAGER OWES A ONE-LINE
  AMENDMENT.** See RULING 1 for the derivation. Recommended wording: the boundary carries **opaque strings
  only — no game-lane include and no Siegebound type in any signature**; `(prompt, gbnf) -> string` is the
  **inference** surface, not an exhaustive enumeration of the boundary. ⛔ **Leaving the clause as-is guarantees
  the next reader files this as a violation** and deletes the only method that makes the mandatory assertion
  implementable. **This is the FOURTH spec line correctly refused this wave (§15) and is filed as a finding
  about the SPEC, not about the code.**

- **[WARN-8 · BOARD-1] `MaxSnapshotChars` IS NOT RETIRED, AND THE REMAINING WORK IS LARGER THAN THE HANDOFF
  STATES.** §8 resolution ⇒1 requires that *"after that commit a grep for `MaxSnapshotChars` returns nothing"*.
  It still resolves: `SiegeAssistantSnapshot.h` (5 lines) · `SiegeAssistantSnapshot.cpp` (8) ·
  `SiegeAssistantComponent.cpp` (2) = **15 matching lines**, against the handoff's *"10 sites"*. **The gap is
  prose-vs-call-site — §14 instance 2 exactly** — so the owed game-lane task must count from a raw `Read`, not
  from a grep. **This is correctly OUT OF SCOPE for a plugin-only task** (the quiet-module law, and the game-lane
  files are dirty from other tasks): ✅ **ratified as a declared departure, not counted against TASK-450.**
  ⇒ **Manager owes one small game-lane task** (retire `MaxSnapshotChars`; set `ZoneBCharReserve` from a
  **printed** reading — and ✅ that reading no longer needs the spike:
  `USiegeAssistantComponent::ReportFirstCapture` prints `zoneB_chars` from the **shipped** builder).

---

## NITS (7)

- **[NIT-1] `SiegeAssistantZoneATest.cpp:603`** — `ExpectedNullLength` is derived from the constant
  `MeasuredZoneAChars` rather than from `WithVocabulary.Len()`. If the shipped lane ever drifts, this test also
  fails, with a message (*"the measured lane with the synonym table swapped for `none`"*) that then misdescribes
  the cause. Using `WithVocabulary.Len()` would make it a pure statement about the **branch**. (Arithmetic
  verified correct as written: `BuildZoneA` emits `synonyms:\n` + the table, newline-ensured, or `none\n` —
  `SiegeAssistantSnapshot.cpp:1052-1072` — and `BuildSynonymTable` already ends in `\n`.)
- **[NIT-2] `SiegeAssistantZoneATest.cpp:251` / `SiegeLlamaSubsystem.cpp:804, 1512`** — `FTCHARToUTF8`,
  `TCHAR_TO_UTF8`, `UTF8_TO_TCHAR`. **Checked in the installed engine: the deprecation at `StringConv.h:1013,
  :1018` is COMMENTED OUT (`5.xx`) — these are NOT deprecated in UE 5.8 and compile clean.** TASK-423's handoff
  item 4 is correct. Recorded only so the next reviewer does not re-litigate it; `StringCast<UTF8CHAR>` is the
  eventual successor.
- **[NIT-3] `SiegeLlamaSubsystem.cpp:1680`** — *"DELETE THIS BLOCK IN THE SAME COMMIT AS SiegeLlamaSpike.cpp"*
  now reads as a **scheduled** deletion; CONVENTIONS §16 rules it **UNSCHEDULED**. Reword when the block is
  next touched.
- **[NIT-4] `SiegeLlamaSubsystem.cpp:1043-1046`** — *"registered before the model finished loading and was
  DISCARDED … register it after `IsReady()`"* is only reachable when the **load failed**, because
  `ApplyPendingStaticPrefix` is called only from the run loop, which runs after `LoadModelGuarded` returns
  (`:530, :547`). A caller who registers early is in fact served correctly. The advice misdirects on the one
  path that can print it.
- **[NIT-5] `SiegeLlamaSubsystem.cpp:1883-1885`** — *"a UObject-bound delegate whose object died … is unbound
  by construction"* holds for `BindUObject`/`BindSP`, **not** for `BindRaw`/`BindLambda` (which the dev command
  uses at `:2034`, safely, with no captures). Narrow the comment to the binding kinds it describes.
- **[NIT-6] `SiegeLlamaSubsystem.cpp:1539`** — on failure the completion still carries the partial `Output`.
  Correct and often useful, but the §9 contract comment does not say callers must key on `bSuccess`. One line.
- **[NIT-7]** `LogSiegeLlama` is **declared in `SiegeLlamaLog.h:27`**, not literally in `SiegeLlamaSubsystem.h`
  as §9's registry block shows. ✅ **RATIFIED**: the header includes it (`SiegeLlamaSubsystem.h:16`), so the pin's
  *purpose* holds transitively; the declaration is **character-for-character identical**; and
  `SiegeLlamaLog.h:15-25` is a standing TASK-409 cross-task instruction telling this task to do exactly this,
  on a checkable mechanism (a second `DEFINE_LOG_CATEGORY` is a **link** error). Confirmed: **zero**
  `DECLARE_/DEFINE_LOG_CATEGORY` in either new file.

---

## THE TASK-424 SPEC'S OWN ELEVEN CRITERIA — item by item (TASK-450)

| # | criterion | verdict |
|---|---|---|
| 1 | **Thread ownership** — one `llama_context`, one thread | ✅ **PASS.** `Model` / `Context` / `Vocab` / `LastPromptTokens` / `StaticPrefixTokenArray` are **private members of `FSiegeLlamaWorker`** (`:419-427`); every dereference is a member function reached only from `Run()`. The subsystem holds a pointer to the **worker**, never the context (`SiegeLlamaSubsystem.h:326`). **I traced every path: no second toucher.** The only foreign-thread code is `AbortThunk` (`:510-514`) and `LoadProgressThunk` (`:517-521`), which read **atomics only**. **No task graph, no `Async()` for inference, no per-request context.** |
| 2 | **Delegate ALWAYS on the game thread, every path** | ✅ **PASS, structurally.** The worker posts a plain `(bool, FString, FString)` **captured by value** via `AsyncTask(ENamedThreads::GameThread, …)` (`:1548-1568`); `ActiveDelegate` is assigned, read and executed **only** on the game thread (`:1809, :1880-1886`), guarded by `check(IsInGameThread())`. **All six exits verified single-post:** `:1180` `:1188` `:1256` `:1271` `:1389` `:1539`, plus the SEH path `:1154`. Holds on success, model-missing, both budget rejections, prefill abort, decode abort, hard timeout, cancel and SEH fault. |
| 3 | **Queue depth 1 by early return, NO queue added** | ✅ **PASS.** `:1766-1772` returns `false` on `IsBusy()`. **There is no queue anywhere in either file.** |
| 4 | **Cancellation via `abort_callback`** | ✅ **PASS.** `ContextParams.abort_callback = &AbortThunk` (`:838`); `CancelActiveRequest` → `RequestCancel()` → `bCancelRequested` → `ShouldAbortNow()` (`:365-373`). **Not a post-generation poll.** The between-slice checks (`:1344, :1437`) are an **addition** that bounds a GPU-tier cancel to one ubatch — not a substitute. ⚠️ **The regime qualifier is real and must be reported by TASK-447** (see RULING 3 / tiering). |
| 5 | **Shutdown / level travel** | ✅ **PASS.** `Deinitialize` → `JoinAndDestroy` (`:1711`) stops the loop, aborts an in-flight **decode** via the same callback **and** an in-flight **model load** via `ShouldContinueLoad` (`:376`, `:801`), `WaitForCompletion`, then frees model+context **on the worker** before `Run()` returns (`:556-561`). `ActiveDelegate.Unbind()` (`:1719`); queued completions hold only a `TWeakObjectPtr` and drop harmlessly. `JoinAndDestroy` is idempotent (the destructor calls it too). **`UGameInstanceSubsystem` ⇒ survives level travel; no reload on Play Again.** |
| 6 | **SEH + `bAssistantFaulted` latch + load failure never blocks match start** | ✅ **PASS.** `SehInvoke` (`:490-507`) wraps **both** the load and every generation, with no destructible locals in the `__try` frame and the reason documented. `LatchFaulted` (`:564-570`) sets a state the `ClearBusy` guard will not clear. Load failure paths at `:1631-1673` each log **once** and return — `Initialize()` never blocks. The absent-model path (`:1649-1655`) is **exactly one Warning**, starts no thread, registers no backends, retries nothing. ⚠️ see **WARN-1** for the one latch hole. |
| 7 | **No game-lane header, no Siegebound type** | ✅ **PASS — verified line by line.** See RULING 1. |
| 8 | **No multi-turn loop, no conversation state** | ✅ **PASS.** `LastPromptTokens = MoveTemp(PromptTokens)` (`:1518`) stores **prompt tokens only** — a KV cache key. **Model output is never fed back**: `OutputBytes` is converted, returned and discarded. Reset on every abnormal exit (`:1385, :1522`) so no turn claims a prefix it cannot prove. |
| 9 | **§9 signatures character-for-character, access levels included** | ✅ **PASS.** Delegate (`SiegeLlamaSubsystem.h:29`), `UCLASS`/`SIEGELLAMA_API`/base class (`:100-101`), `IsReady` (`:228`), `IsBusy` (`:231`), `RequestCompletion` (`:243`), `CancelActiveRequest` (`:260`) — **all `public:` and byte-identical to `CONVENTIONS.md:829-839`.** Additions are additive only. See **NIT-7** for `LogSiegeLlama`. |
| 10 | **Spike harness DELETED** | ⛔ **SUPERSEDED BY CONVENTIONS §16 — NOT ENFORCED, AND ENFORCING IT WOULD BE THE DEFECT.** See RULING 4. |
| 11 | **Standing coding laws + M8 declaration verbatim** | ✅ **PASS.** *"adds no replicated property, no new replicated class, no new relevancy tier"* — verbatim in `handoffs/TASK-450-programmer.md:4`, `handoffs/TASK-423-programmer.md:4`, `SiegeLlamaSubsystem.h:97-98` and `SiegeAssistantZoneATest.cpp:17-18`. Files land at the exact §5 paths. Console commands are `FAutoConsoleCommand*` in the `Siege.Llama.*` namespace under `#if !UE_BUILD_SHIPPING` (`:1918-2103`) — **never a `UFUNCTION(exec)` on a gameplay class.** |

**Compile-gate risk swept (TASK-447 is the one gate, so a link error here is expensive):** `Engine` is in
`PublicDependencyModuleNames` (`SiegeLlama.Build.cs:13`) — required by `UGameInstanceSubsystem`. All four
cross-file plugin symbols exist: `FSiegeLlamaModule::GetPtr` (`SiegeLlamaModule.h:33`), `IsLlamaAvailable`
(`:44`), `GetLoadError` (`:47`), `EnsureBackendsLoaded` (`:59`), `USiegeLlamaSettings::ResolveModelPath`
(`SiegeLlamaSettings.h:83`). Console API verified in the installed engine: class at `IConsoleManager.h:2467`,
delegate at `:263` with the exact `(const TArray<FString>&, UWorld*, FOutputDevice&)` signature used.
`TestEqualSensitive` / `TestNotEqualSensitive` `FString` overloads at `AutomationTest.h:2016` / `:2023`.
`UCLASS`/`GENERATED_BODY`/`.generated.h`-last are correct; **no reflection macro is needed or present** (the
two `UPROPERTY`/`UFUNCTION` hits in the new files are both inside comments). **No GC hazard:** no raw `UObject*`
members; the worker holds a `TWeakObjectPtr` back-reference.

---

## NOTES FOR BUILD-MASTER (TASK-447) — FOUR QUALIFICATIONS, ALL BINDING

1. ⛔ **`Siegebound.Assistant.ZoneA.*` GREEN IS WARN-5's DISCHARGE EVENT — CLAIM IT AGAINST THE RUN, NEVER
   AGAINST THIS REPORT OR A HANDOFF.** And even green, three things stay out of reach and must not be claimed:
   the **`DA_AssistantVocabulary` ASSET lane** (untested — the asset overrides the C++ defaults wholesale;
   TASK-452 owns it), **tokens** (a runtime reading, never asserted here), and *"the lanes were equal when the
   5116 run was taken"* (this proves they are equal **now**).
2. ⚠️ **RECORD `LLM_LOAD: ggml backends ready in N ms`.** *"Async load never blocks match start"* is
   **QUALIFIED-PASS + N**, never a bare PASS. A large N is a finding for Jonathan.
3. ⚠️ **STATE THE TIER ON THE CANCEL RUN.** The policy selects **PARTIAL** on this machine (3320.2 MiB free vs
   the 3471 full threshold), so `Siege.Llama.Test long cancelms=200` proves the **ubatch-boundary** path, not
   the mid-graph one. `abort_callback` is CPU-only (`llama.h:384`). ⛔ A cancel measured on a GPU tier is not
   evidence for the mid-prefill claim.
4. ⛔ **THE PARTIAL-TIER FRAME-TIME HITCH RE-MEASURE IS NOT SATISFIABLE FROM THE SUBSYSTEM.** The frame sampler
   and hitch histogram exist **only** in `SiegeLlamaSpike.cpp`; the subsystem has no frame instrumentation and
   was never specced to. **Run it on the spike in its own session, or record it NOT MEASURED.** ⛔ **Never
   substitute a number from a different instrument** (§12h's sibling problem).
   ⚠️ **And do NOT drive `Siege.Llama.Spike*` and `Siege.Llama.Test` in the same session** — two ~2.5 GB models
   on one card. The subsystem warns about this at load (`:1696-1698`).
   **Suggested gate order:** `Siege.Llama.Info` → `.Status` → `.Test` → `.Test twice` (**pass = call 1 TRUE,
   call 2 FALSE; two FALSEs proves nothing**) → `.Test long cancelms=200` → rename the `.gguf` → restart →
   `.Status`.
5. **Commit scope:** the two plugin files + `SiegeAssistantZoneATest.cpp` + `qa/TASK-424.md` + handoffs + board.
   ⛔ **`SiegeLlamaSpike.cpp` IS NOT DELETED IN THIS COMMIT** (CONVENTIONS §16). **`L_Arena` never saved** ·
   stage by explicit pathspec · real hashes.

---

## BOARD FLIPS OWED (I hold no edit tool — orchestrator to apply)

- **BOARD-1:** TASK-423 → **`qa-passed`**. TASK-450 → **`qa-passed`**. TASK-424 → **`done`**
  (report at `.claude/pipeline/qa/TASK-424.md`).
- **BOARD-2:** ⛔ **Strike TASK-424 spec item (10)** (*"the spike harness is DELETED … is a BLOCKER"*) as
  **superseded by CONVENTIONS "Settings screen…" §16**, and mark TASK-447's inherited *"the spike-harness
  deletion in the same commit"* clause **VOID**. Enforcing either would spend the sealed gen-2 holdout.
- **BOARD-3:** **Strike `SpikeCorpus.inl` from TASK-423's and TASK-450's `names:` lines** — confirmed by
  filesystem glob that it has never existed. §16:1377 already struck it from CONVENTIONS.
- **BOARD-4:** Record **WARN-7/SPEC-1** as a manager-owed one-line amendment to CONVENTIONS §8, and
  **WARN-8/BOARD-1** as the owed small game-lane task (`MaxSnapshotChars` retirement at **15 matching lines**,
  not 10; `ZoneBCharReserve` from `ReportFirstCapture`'s printed `zoneB_chars`).
- **BOARD-5:** WARN-1 … WARN-6 are **non-blocking**; they are cheap and should be batched into the first
  game-lane task that opens `SiegeLlamaSubsystem.cpp` (the FSM caller), **not** dispatched individually.

## ✅ CREDIT WHERE IT IS DUE — recorded, because this is the behaviour the pipeline wants

Three separate times in this batch a programmer **declared a shortfall or refused a spec line on a stated,
checkable mechanism instead of quietly complying**: TASK-423 declaring the partial delivery, TASK-450 declaring
the `SetStaticPrefix` contradiction and the spike dependency, and TASK-450 declaring `EnsureBackendsLoaded`'s
synchronous call as *"the weakest point in my own claim"*. **Every one was verified correct at the artifact by
this review.** §15 says QA **ratifies** such departures rather than filing them as deviations — and it is the
only reason a sealed corpus is still measurable and the compile gate is not about to be handed a partial.

---

# ⛔ APPENDED BY BUILD-MASTER 2026-08-03 — TASK-447 COMPILE GATE: **FAILED**. QA LOOP 1 OPENED ON **TASK-450 ONLY**.

**Appended, not edited in. Nothing above was altered.** Author: build-master. ⛔ **I did not fix the
code.** TASK-423's deliverable is **not** implicated — see the credit section below.

## Verdict, quoted from the log (`scratchpad/build2.log`)

```
Result: Failed (OtherCompilationError)
Total execution time: 14.21 seconds
```
⛔ `$LASTEXITCODE` was `6` and was not used (§17).

## ⛔ THE DIAGNOSTIC — TASK-450, `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`

**ONE error, and it is unambiguous. Quoted in full with the engine's own note:**

```
SiegeLlamaSubsystem.cpp(285,15): error C2555: 'FSiegeLlamaWorker::Run': overriding virtual function
return type differs and is not covariant from 'FRunnable::Run'
    virtual void Run() override;
                 ^
Engine/Source/Runtime/Core/Public/HAL/Runnable.h(45,17): note: see declaration of 'FRunnable::Run'
    virtual uint32 Run() = 0;
                   ^
```

⇒ **`FSiegeLlamaWorker::Run` is declared `virtual void Run() override;`. `FRunnable::Run` is
`virtual uint32 Run() = 0;`.** The return types differ and `void`/`uint32` are not covariant, so the
override is ill-formed. ⚠️ **This is a pure signature mismatch against the engine base class** — it is
not a logic defect, and nothing about the worker's *behaviour* is implicated by it.

⛔ **THE FIX IS THE PROGRAMMER'S, AND IT IS NOT PURELY MECHANICAL — WHICH IS EXACTLY WHY I AM NOT
DOING IT.** Changing the return type to `uint32` forces a **`return` value on every exit path of the
definition**, and `FSiegeLlamaWorker::Run`'s body is the request loop this batch has been auditing.
⚖️ **§20: a fix named in a report is a HYPOTHESIS the implementer must trace** — in particular against
`bWorkPending` / `bAborted` / `MarkBusy()` ordering, which **TASK-457 deliberately reordered and QA
ratified**. **Do not let a return-type edit disturb that.**

⚠️ **§22 — SWEEP THE SHAPE, DO NOT PATCH THE LINE.** Check the **definition** as well as the
declaration at `:285`, and check `Stop()` / `Init()` / `Exit()` against `FRunnable` at the same time.
**A build reports only what it reached.**

## ✅ WHAT THIS BUILD PROVES IN TASK-450's FAVOUR

- ✅ **`Module.SiegeLlama.cpp` COMPILED** (action 9 of 23) ⇒ **the plugin's UHT surface is valid.**
- ✅ **TASK-459's and TASK-457's edits are NOT implicated.** The only diagnostic in this file is at
  `:285`, a class declaration. **`RunRequestUnguarded`'s two timeout arguments (`:1572` / `:1725`),
  both guards and the `bAborted` extension produced no diagnostic** — consistent with `qa/TASK-461.md`
  §8.1's prediction that `double`→`double` into an unmodified `%.1f` introduces no compile risk.
  ⛔ **TASK-459 and TASK-457 are NOT in a QA loop and their statuses do not change.**
- ✅ **`GitClaudeUnrealTest.Build.cs`'s `"SiegeLlama"` dependency RESOLVES** — the game module's
  `#include "SiegeLlamaSubsystem.h"` produced no diagnostic in any of the 16 game-lane TUs that built.

## ✅ TASK-423 IS NOT IMPLICATED — its deliverable COMPILED

`SiegeAssistantZoneATest.cpp` built clean (action 6 of 23). ⛔ **TASK-423's status does not change.**
⚠️ **But `Siegebound.Assistant.ZoneA.*` has still NEVER RUN — WARN-5 remains INSTRUMENTED, NOT
DISCHARGED,** and this build moves it not at all. **Compiling is not passing.**

## ⛔ WHAT THIS BUILD DOES NOT PROVE

1. ⛔ **THE LINK NEVER RAN** (stopped at 18 of 23 actions) — no unresolved-external evidence either way.
2. ⛔ **NOT NECESSARILY THE COMPLETE ERROR SET** — a failing TU stops at its own errors and 5 actions
   never started. ⛔ **"Fix this one and it builds" is not a claim made here.**
3. ⛔ **NOTHING EXECUTED** — no smoke test, no `Siege.Llama.*` command, no deadline exercise, no
   `EnsureBackendsLoaded()` timing. **The Zone B reading is still NOT MEASURED, with no substitute.**

## Board flips owed (orchestrator applies)

- **TASK-450** → ⛔ **`qa-failed`** (this report). **QA loop 1 of 3.**
- **TASK-423 · TASK-457 · TASK-459** → ⛔ **NO CHANGE** — all compiled clean.
- **TASK-447** → remains open, **not `done`**. Nothing committed, nothing pushed.
