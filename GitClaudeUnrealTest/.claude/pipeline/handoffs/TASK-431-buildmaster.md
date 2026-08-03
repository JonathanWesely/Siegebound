# TASK-431 — COMPILE GREEN · THE DEV RE-SCORE (⚖️ THE LADDER GATE) · THE CPU-ABORT DIAGNOSIS (build-master)

**Date:** 2026-08-03 · **Branch:** `main` · **HEAD at run:** `66c6854` · **No Git, no commit, no push** (TASK-434 owns the commit).

---

## 0. THE VERDICT, FIRST, IN ONE BLOCK

| question | answer |
|---|---|
| **Does the dev gate pass?** | ⛔ **NO. BOTH HALVES FAIL.** |
| **May TASK-432 spend the sealed holdout?** | ⛔ **NO. HOLDOUT 2 STAYS SEALED.** |
| Compile | ✅ **GREEN**, both lanes, zero diagnostics |
| `zoneA_tok` (**MEASURED**) | ✅ **1356** vs ceiling **1389** ⇒ **33 slack. UNDER BUDGET.** |
| t0 tripwire | ✅ `zoneB_chars=68` `zoneC_chars=887` — **both hold exactly** |
| Dev score | ⛔ **19/25 = 76.0 %** (gate 22/25 = 88 %) — **3 rows short** |
| Refuse-class failures | ⛔ **1 (DEV-04)** — gate requires **ZERO**. **CATEGORICAL FAIL.** |
| CPU aborts | ✅ **A TUNABLE, NOT A WALL** — proven, not asserted |

⇒ **Route back to `gameplay-programmer`. This is LADDER LOOP 1 of 3.**
⚠️ **The CPU diagnosis (§5) was run ONCE on this first pass, per board §(4). DO NOT re-run it on a ladder loop.**

---

## 1. STEP 0 — COMPILE

**Judged on the log's `Result:` line, never the exit code.** (Exit code happened to agree this time — it was `0` and the build genuinely succeeded — but it was read as non-evidence, per the standing law that `Build.bat` has been wrong in both directions on this batch.)

```
Result: Succeeded
Total execution time: 15.61 seconds
```

**It is a REAL compile, not a no-op** — the log names every changed translation unit and both link steps:

```
[Adaptive Build] Excluded from SiegeLlama unity file: SiegeLlamaSpike.cpp
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegeAssistantSnapshot.cpp, SiegeAssistantVocabulary.cpp
[2/12] Compile [x64] SiegeAssistantVocabulary.cpp
[3/12] Compile [x64] SiegeAssistantSnapshot.cpp
[4/12] Compile [x64] SiegeLlamaSpike.cpp
[6/12] Link [x64] UnrealEditor-SiegeLlama.dll
[11/12] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
```

- **Both lanes: plugin (`SiegeLlama`) + game module (`GitClaudeUnrealTest`).**
- **Zero diagnostics** — no warning, no error, under warnings-as-errors.
- ✅ All three changed files were compiled **outside unity** (adaptive non-unity, because they are the Git working set). That is the **stricter** path — no unity blob could mask a missing include.
- ✅ Editor confirmed running the fresh binaries: both DLLs stamped **2026-08-03 03:46**, editor PID 31460, world `L_Arena`.
- 📌 **UBT independently reports `16 physical cores, 16 logical cores`** — corroborating `DefaultThreadCount() = max(1, 16-2) = 14` from the code side rather than from the logs.

**Machine/procedure notes:** editor closed gracefully via `WM_CLOSE` (the same signal as clicking the X — **not** a force-kill; `Stop-Process` was never used), verified exited cleanly with nothing saved. §12e's remote-exec channel was restored on relaunch via the **launch-flag lane** (`-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=True,...`), **not** by hand-editing `Saved/Config/WindowsEditor/Engine.ini` — that file's hand-edit lane is hook-blocked (TASK-350 precedent) and the launch flag is zero-file-edit, so **there is no config residue to restore.**

---

## 2. STEP 1 — ZONE A, MEASURED. ⚖️ THE §12c HARD GATE IS CLEARED ON A MEASUREMENT.

`Siege.Llama.SpikePrompt` (no `order=` override, so the in-code tripwire is armed), model resident:

```
SPIKE_TOKENS fixture=t0 zoneA_chars=5108 zoneB_chars=68 zoneC_chars=887 zoneBC_chars=955 MaxSnapshotChars=1085 headroom=130
SPIKE_TOKENS fixture=t0 zoneA_tok~=1356 zoneB_tok~=32 zoneC_tok~=320 zoneBC_tok~=352 (<=400 cap) assembled_total_tok=1721 chars_per_token=3.57
SPIKE_TOKENS fixture=t0 assembled_prompt_chars=6143 zoneB_start_char=5155 zoneC_start_char=5223 zoneB_start_tok~=1364 zoneC_start_tok~=1396
```

### ⚖️ `zoneA_tok~= 1356` against the ceiling **1389** ⇒ **33 tokens of slack. UNDER BUDGET.**

⚠️ **It did NOT overshoot, and the direction of the error is the point.** Slack was *claimed* at **18** (the DERIVED 1371). The **measured** figure is **1356** — **15 tokens BELOW the derivation**.

⇒ **The blend QA ruled sound at §5b was CONSERVATIVE, not optimistic.** The derivation over-estimated the cost of the added text, which is the direction §8's law demands. QA's ruling for the blend over the flat 2.71 reading (which would have predicted **1432**, over by 43) is **vindicated by measurement**: the flat-2.71 reading was wrong by **76 tokens**, the blend wrong by **15**, and only in the safe direction.

- ⛔ **No contingency cut is owed.** The `archer != longbowman` note and the counts rule both stay.
- ✅ `zoneA_chars=5108` — **exactly** the hand-derived figure from both the tuner (§R2a) and QA (§R5). The char arithmetic was right to the byte.
- ✅ `zoneBC_tok~=352` inside the 400 cap; `headroom=130` chars.
- ⚠️ **NIT-5 confirmed live:** the `Reserve` comment at `SiegeAssistantSnapshot.cpp:614-615` still says **5111**; the truth is **5108**. Comment-only, no byte emitted. Left alone deliberately — not worth a compile.

### ✅ THE t0 TRIPWIRE HOLDS — `zoneB_chars = 68`, `zoneC_chars = 887`

**Both exact.** The in-code tripwire (`SiegeLlamaSpike.cpp:4637`) **did not fire**, and no `SPIKE_WARN` of any kind appeared in the prompt or load runs. **No fixture moved. Bar #3's reproduction and the corpora's premises are intact.**

### ✅ WARN-6 DISCHARGED — the few-shots were PARSED, not merely read

- **`parse_failures = 0`** across all 25 dev rows.
- **Zero occurrences** of `llama_sampler_init_grammar returned NULL` (the harness logs that at `Error` level, `SiegeLlamaSpike.cpp:3033-3038`) ⇒ **the grammar sampler was non-NULL for the whole run.**

⇒ **This was the first time the four new few-shots were ever put through the sampler, and they parsed clean.** That is the acceptance evidence §9c demanded, and it is now delivered as a parse rather than a reading. **A dump is not a parse — this was a parse.**

---

## 3. STEP 2 — ⚖️ THE DEV GATE. **NOT MET, ON BOTH HALVES.**

`Siege.Llama.SpikeEval tier=full gpu=0` — `gpu=0` verified as the RTX 5070 on the load line:
`device=dev=0/Vulkan0/GPU ... desc="NVIDIA GeForce RTX 5070 Laptop GPU"`.
⛔ **`holdout=` was never passed.** The harness itself confirms: `SPIKE_RUN: no holdout= given, so ONLY the dev split was scored.`

### Reported exactly as printed

```
SPIKE_EVAL_SCORE split=dev rows=25 PRIMARY(lenient)=19/25 = 76.0% | STRICT=19/25 = 76.0%
                 | parse_failures=0 | clarify_rows_passed_by_question=0 | assertions_compared=70
SPIKE_EVAL_SCORE split=dev deferred-trigger agreement (NOT part of bar #5's four fields) = 1/1
SPIKE_EVAL_SCORE split=dev LENIENCY FLOOR: a degenerate model that answers {"ask":...} to EVERY sentence
                 would score 12/25 = 48.0% under the PRIMARY rule (it fails every Execute row).
SPIKE_EVAL_SCORE split=dev 6 row(s) assert at most ONE field and therefore test less than the score implies:
                 DEV-04 DEV-06 DEV-08 DEV-09 DEV-10 DEV-25
SPIKE_EVAL_SCORE split=dev 1 row(s) assert NOTHING AT ALL and pass on any parseable output: DEV-08
```

### The two halves

| half | requirement | measured | verdict |
|---|---|---|---|
| **(i)** | **≥ 88 % (22/25)** | **19/25 = 76.0 %** | ⛔ **FAIL — 3 rows short** |
| **(ii)** | **ZERO Refuse-class failures** | **1 Refuse row emitted a live executable order** | ⛔ **CATEGORICAL FAIL** |

**Baseline was 18/25 = 72.0 %. The wave moved the number by exactly ONE ROW.** Predicted was 22–24.

### ⛔ HALF (ii) — THE REFUSE CLASS, CALLED OUT SEPARATELY

**All three Refuse rows, individually:**

| row | expect | got | verdict | what it emitted |
|---|---|---|---|---|
| **DEV-04** | Refuse | **`command`** | ⛔ **FAIL — LIVE ORDER** | `{"intent":"send","who":[{"kind":"sorcerer","n":1}],"where":"enemy_castle","when":"now"}` |
| DEV-09 | Refuse | `question` | ✅ **PASS** | — |
| DEV-10 | Refuse | `question` | ✅ **PASS** | — |

**DEV-04 sentence:** `send the catapults at the enemy base`

⚠️ **THE SUBSTITUTION ATTRACTOR MOVED BUT DID NOT GO AWAY.** Baseline substituted `sapper`; it now substitutes **`sorcerer`**. The rule *"Never swap in a listed kind"* plus the `werewolves` exemplar **did not stop the substitution** — they only changed which listed kind gets swapped in. This is class (a), the class **QA named WEAKEST at §2 and was right about.**

✅ **The good half:** baseline had **2 of 3** Refuse rows emitting live orders (out-of-roster → `sapper`; card play → `ambush knight`). It is now **1 of 3**. **The card-play/economy refusal is genuinely FIXED** — which is the one that violated Jonathan's standing ruling that the AI never spends gold. That ruling is no longer being broken on dev.

⛔ **But the gate says ZERO, and zero is not 1.** Half (ii) is categorical and not tradeable, so **even a 22 would not have opened the holdout with DEV-04 in this state.**

### The per-row diff — every failure

| row | expect | broke | sentence | raw output |
|---|---|---|---|---|
| **DEV-01** | Execute | `where=BAD` | `send 10 footmen with a sorcerer to the nearest ancient ground` | `{"intent":"send","who":[{"kind":"footman","n":10},{"kind":"sorcerer","n":1}],"where":"nearest_mine","when":"now"}` |
| **DEV-04** | **Refuse** | `intent=BAD` | `send the catapults at the enemy base` | `{"intent":"send","who":[{"kind":"sorcerer","n":1}],"where":"enemy_castle","when":"now"}` |
| **DEV-07** | Clarify | `kinds=BAD counts=BAD` | `charge with the footmen` | `{"intent":"charge","who":"none","where":"none","when":"now"}` |
| **DEV-11** | Execute | `counts=BAD` | `wait until i have 2 more knights then send them mid` | `{"intent":"send","who":[{"kind":"knight","n":"all"}],"where":"mid","when":{"kind":"knight","at_least":2}}` |
| **DEV-16** | Execute | `intent=BAD kinds=BAD counts=BAD` | `infantry back to our castle now` | `{"intent":"fallback","who":"none","where":"own_castle","when":"now"}` |
| **DEV-20** | Execute | `counts=BAD` | `clerics follow me` | `{"intent":"follow","who":[{"kind":"cleric","n":1}],"where":"hero","when":"now"}` |

### ⚖️ THE CLASS SCORECARD — only ONE of the four taught classes landed

| class | QA's §2 ranking | **measured outcome** |
|---|---|---|
| **(b) ECONOMY / CARD-PLAY** | PASS (graded hardest) | ✅ **LANDED** — DEV-09 + DEV-10 both pass |
| **(a) OUT-OF-ROSTER REFUSAL** | ⚠️ WEAKEST | ⛔ **STILL BROKEN** — DEV-04 |
| **(c) SELECTION-PRESERVING** | ✅ **STRONGEST** | ⛔ **STILL BROKEN** — DEV-07, DEV-16 (`who:"none"` both times) |
| **(d) DETERMINER → QUANTITY** | ✅ PASS | ⛔ **STILL BROKEN** — DEV-20 (`n:1` for a bare plural), DEV-11 |

⚠️ **QA's class ranking was inverted by measurement on its most confident call.** Class (c), rated **STRONGEST** on public evidence, failed **twice** — and both failures are the same shape: the model drops the named selection to `who:"none"` instead of preserving it. **That is the single highest-value lead for the next rung**, and it is worth more than DEV-04 because it is two rows, not one.

### ⚠️⚠️ THE WARN-2 READING RULE — QUOTED VERBATIM AS REQUIRED

> **A 22 or a 23 means "AT THE GATE WITH THE INFORMATIVE ROWS UNPROVEN" — NOT "passed with room."**

**And the measurement makes the question moot in the hardest possible direction: `DEV-01` FAILED.**

`DEV-01` is the row few-shot #1 near-paraphrases (two tokens apart, nine content words shared) **and** the row the one new alias `nearest ancient ground` directly targets — two reasons, one row. **It did not flip.** The model still resolves `the nearest ancient ground` to **`nearest_mine`**.

⇒ Three consequences, stated plainly:

1. **The 19 contains ZERO contribution from the structurally uninformative row.** Nothing in this score is inflated by the double-taught row — the number is, if anything, *honest to a fault*.
2. **My recommendation stands on neither of the two readings**, because the question only arises at 22–23 and we are at **19**. There is no "at the gate" call to make: the gate is missed by 3 rows and by a categorical failure.
3. ⚠️ **The place-collision defect is worse than the tuner's own §7.2 prediction.** The tuner predicted `closest ancient ground` would still fail but expected the pinned alias `nearest ancient ground` to work, at **"high" confidence**. **The alias did not fix even its own exact target string.** Removing `gold` from `nearest_mine` and adding the alias were both correct edits — QA's §3 ruling on the `gold` deletion still stands — but **the `nearest_mine` attractor is stronger than an alias can overcome**, and that is a new, measured finding this run produced.

### ⚠️ DEV-21 — THE WARN-7 OVER-REFUSAL TRIPWIRE. **ACQUITTED, ON EVERY CHANNEL QA NAMED.**

```
id=DEV-21 expect=Clarify got=command lenient=PASS strict=PASS asserted=4 compared=4 skipped=0
          intent=ok kinds=ok counts=ok where=ok
id=DEV-21 sentence="send the ogre at their castle"
          raw={"intent":"send","who":[{"kind":"ogre","n":1}],"where":"enemy_castle","when":"now"}
```

- ✅ **DEV-21 passes STRICT and LENIENT, on all four fields.** It emitted a **live `ogre` order** — the opposite of a refusal.
- ✅ **NO STRICT/LENIENT DIVERGENCE ANYWHERE:** `PRIMARY(lenient)=19/25` and `STRICT=19/25` are **identical**. QA named divergence as the fingerprint; there is none, on any row.
- ✅ **`clarify_rows_passed_by_question = 0`** — the second fingerprint QA named. Also zero.

⇒ **WARN-7 is acquitted by measurement.** The `werewolves` exemplar's semantic proximity to `ogre` did **not** bleed refusal onto the live neighbouring kind. The tuner's R1b.3 "refusal under temptation" choice cost nothing on the detector QA built for it. **The sharper exemplar was the right call and it is now measured, not argued.**

⚠️ Note the exemplar also demonstrably did **not** achieve its purpose (DEV-04 still substitutes). **It is harmless, not effective** — those are separate findings and both are true.

### ✅ NOTE-5 CHECK — this ladder loop is legitimately routed

**`ROW_DID_NOT_COMPLETE` count in the dev eval: 0.** Every one of the 25 rows ran to a natural stop on `tier=full`. **No failure in the table above is a stopped generation** — they are all genuine comprehension failures. ⇒ **Routing this back to the programmer does not burn a loop on a decode-rate problem**, which is exactly the trap NOTE-5 and TASK-429's new field exist to prevent.

---

## 4. ⛔ THE HOLDOUT SEAL — INTACT, AND VERIFIED RATHER THAN ASSERTED

- **No `holdout=` argument was passed to any command in this task.**
- The harness printed its own confirmation: `SPIKE_RUN: no holdout= given, so ONLY the dev split was scored.`
- Swept every run log from this task: **zero `split=HOLDOUT` lines, zero `HOLD-` row ids.**
- Neither `assistant_eval_holdout.csv` (spent) nor `assistant_eval_holdout2.csv` (sealed) was opened, by any command or by me.

⇒ ✅ **TASK-432's one-shot is unspent and remains available.**

---

## 5. STEP 3 — ⚖️ THE CPU-ABORT DIAGNOSIS (Jonathan asked for this personally)

**Pre-flight satisfied before any bench:** PIE on `L_Arena`, **39 units fielded on both sides** via the shipping `SummonTestUnit` path (never an empty map), **measured live FPS settled at 59.7** (≥ 58 bar) *after* the spawn burst, throttle line `bThrottleCPUWhenNotForeground=False` verified in the user-global `EditorSettings.ini`. **Every cell run twice; the SECOND is reported.**

### 5a. ⛔ THE THREAD HYPOTHESIS IS DEAD — the actual is NOT clamped

```
SPIKE_LOAD tier=cpu gpulayers=0(req)/36 ctx=2048(req 2048) batch=512(req 512) ubatch=64(req 64)
           threads=14(req 14) threads_batch=14(req 14) cores=16 load_ms=564 model=Qwen3-4B-Q4_K_M.gguf
```

- **ACTUAL = 14. REQUESTED = 14. `cores=16`. They are EQUAL.**
- **`SPIKE_WARN: THE CONTEXT CLAMPED THE THREAD REQUEST` did not fire — in any cell of the matrix.**
- Pinning explicitly with `threads=16` is **also honoured**: `threads=16(req 16) threads_batch=16(req 16) cores=16`.

⇒ ⚖️ **There is NO configuration bug and NO clamp. The CPU tier really is running on the 14 cores it asked for.** The dispatch's *"if the actual is clamped far below, that is a bug and a root cause"* branch **does not apply** — and it does **not** settle Jonathan's question. The instrument TASK-429 built did its job: it converted an unobservable into a measured negative, which is a real result even though it is not the convenient one.

⚠️ **And more threads is WORSE, not better** — so thread starvation is not a hidden second cause:

| cell | threads | **mean ms/token** | mean wall | worst wall |
|---|---|---|---|---|
| (b) `deadline=20` | **14** | **174.88** | 9461.0 ms | 13270.4 ms |
| (c) `deadline=20 threads=16` | 16 | **188.58** | 9909.9 ms | 12737.8 ms |

**Oversubscribing all 16 cores costs ~8 % per token** (the render thread and the game are on the same machine). ⇒ **`DefaultThreadCount()`'s "leave two cores for the game" is empirically the right call and should not be changed.**

### 5b. ✅ THE ANSWER: **THE ABORTS ARE A TUNABLE, NOT A WALL.** Proven by A/B.

**(a) CONTROL — `deadline=10` (the shipped ceiling), threads=14. THE DEFECT REPRODUCES:**

```
SUMMARY iters=5 aborted_or_failed=2/5 deadline_s=10.0(default = CONVENTIONS section 10 HardTimeoutSeconds,
        AUTHORITATIVE: gpulayers=0, so abort_callback aborts llama_decode mid-graph per llama.h:382-385)
        mean_ttft_ms=2572.0 mean_wall_ms=8727.0 WORST_wall_ms=10003.2 total_out_tokens=179
```
(pass 1 of the same cell: `aborted_or_failed=1/5` — the defect is **stochastic between 1 and 2 of 5**, which is itself worth recording.)

**(b) DEADLINE SWEEP — `deadline=20`, everything else identical. THE DEFECT VANISHES:**

```
SUMMARY iters=5 aborted_or_failed=0/5 deadline_s=20.0(OVERRIDDEN by deadline= ...)
        mean_ttft_ms=2557.1 mean_wall_ms=9461.0 WORST_wall_ms=13270.4 total_out_tokens=194
```
**`0/5` on BOTH passes.** `gpu=0` was held identical to (a) on purpose — `deadline=` is not in the reload key, so **the ceiling was the only variable between the two cells.**

**Three independent lines of evidence that this is a cut, not a failure:**

1. **The aborts land at output tokens 41–46**, always `llama_decode=2` (= *aborted*, `llama.h:963-977`), always with `wall_ms` pinned to **exactly the ceiling** (10000.4 / 10003.2 / 10001.7 ms). A generation that *fails* does not fail at precisely 10.000 s.
2. **The truncated JSON is CORRECT UP TO THE CUT.** The same iteration that aborted at 45 tokens under `deadline=10` completes at **57 tokens** under `deadline=20`. Its cut output reads:
   `{"intent":"send","who":[{"kind":"footman","n":12},{"kind":"archer","n":2},{"kind":"sorcerer","n":1}],"where":"ancient_ground_far","when":{"` — **a model that understood the sentence and ran out of clock**, not one that failed.
3. **The arithmetic the board asked me to prove or break — CONFIRMED.** Measured TTFT **~2.56 s** + 57 tokens × **~175 ms/token** ≈ **12.5 s**, against a **10.0 s** ceiling. The hypothesis was right, and it is now measured rather than inferred.

### 5c. The deadline regime — reported as asked

The log states its own regime on every line. On the CPU tier it reads **`AUTHORITATIVE: gpulayers=0, so abort_callback aborts llama_decode mid-graph per llama.h:382-385`**; on the GPU tier (the dev eval) the same field printed **`ADVISORY ONLY: layers are on the device and abort_callback is documented CPU-only ... tokens= is what actually bounds this run`**.

⇒ **Every deadline figure in §5 is from the AUTHORITATIVE regime.** The knob genuinely bounds these runs; it is not advisory here.

### 5d. ⚠️ WHAT I CANNOT MEASURE, STATED RATHER THAN IMPLIED

**`threads=` moves `n_threads` and `n_threads_batch` TOGETHER** (`ContextParams.n_threads_batch = ContextParams.n_threads`). Prefill and decode are driven by the same pool and there is **no `threads_batch=` argument**.

⇒ ⛔ **The experiment that would separate the CPU tier's TTFT cost from its decode cost CANNOT BE RUN with the levers that exist.** The harness *reports* the split per iteration (TTFT ~2.56 s vs decode ~3.0–10.9 s), so the two costs are **observable** — but they are **not independently attributable or tunable**, and I am not going to imply otherwise. Adding that lever was not specced and I did not add it.

⚠️ **`wall60_equiv_ms` is printed on every iteration and is an EXTRAPOLATION. It is quoted NOWHERE in this report as a measurement.**

### 5e. ✅ WARN-R2 REGRESSION CHECK — TASK-429's FIX HOLDS, TWICE

Board §(5) asked whether the layout verdict fires on a decode abort. **It does not.** Two live aborts hit the prefill bound during this matrix and both were correctly reported as aborts:

```
DROP=98.8% [NOT A MEASUREMENT -- turn 1 did not complete (DECODE llama_decode=2 at output token 56 ...),
so turn 2 re-prefilled from a cleared cache basis. This figure is an artifact of the abort and is NOT a bar #3
result. NO LAYOUT VERDICT IS RENDERED]

SPIKE_WARN: bound=BEST_CASE DID NOT COMPLETE BOTH TURNS, so ITS LAYOUT VERDICT IS SUPPRESSED ...
THE FINDING HERE IS THE ABORT, NOT THE PROMPT LAYOUT
```

**The string *"THE PROMPT LAYOUT IS WRONG"* never appeared in any run.** The `BAR#3 BOUNDS` summary line also correctly carried `NOT A BAR #3 RESULT`. ⇒ ✅ **No regression. The diagnostic that misled twice now names the right component.** Per §12e no bar #3 figure is quoted from any incomplete bound in this report.

### 5f. 🧑 THE RECOMMENDATION TO JONATHAN — three lines, a recommendation and NOT a decision

⛔ *I did not change `HardTimeoutSeconds`, did not edit §10, and do not restate the requirement — ruling (i) stands until he moves it (ruling 9).*

1. **WHAT THE ABORTS ARE.** ✅ **A deadline artifact — a TUNABLE, not a wall, and not a thread bug.** The CPU tier produces **correct, complete, grammar-legal JSON for every fixture** when given time; the shipped 10.0 s ceiling cuts the longer answers mid-decode. The threads are not clamped (14 of 16, exactly as requested) and giving it all 16 makes it slower.
2. **WHAT IT COSTS TO MAKE THE CPU TIER RELIABLE.** Raising the ceiling **10 s → 20 s** gives **0 aborts in 10 of 10 iterations across two passes.** The resulting worst-case wall measured **13.3 s** (and **15.8 s** on a noisier pass of the 16-thread cell), with mean wall **~9.5 s** and mean TTFT **~2.6 s**. So the honest cost is: **a ~20 s ceiling, a ~9.5 s typical wait, and a worst case near 16 s.**
3. **WOULD I SHIP ON IT?** ⚠️ **The requirement survives on CORRECTNESS and fails on LATENCY — and only the second half is a judgement call.** Raising the deadline converts *unusable truncated output* into *correct but slow output*, which is a genuine and cheap improvement I would take regardless. **But it does not make the CPU tier meet bar #2**: §12d's bar governs wall-clock at **≤ 6000 ms cpu**, and the abort-free CPU tier measures **~9.5 s mean / ~13.3 s worst — missing it by ~1.6×**, exactly as it did at TASK-413. ⇒ **My recommendation: keep CPU fallback as a CORRECTNESS guarantee (it demonstrably works), and treat "CPU meets the latency bar" as the thing that is not true.** Whether a ~10-second single-order wait is shippable as a fallback experience is **yours to rule on, not mine** — but you should rule on it knowing the fallback *works* and is merely slow, which is a materially different question from the one the aborts made it look like.

---

## 6. LEDGER FROM THIS RUN

- ✅ **WARN-6 DISCHARGED** — `parse_failures=0` + non-NULL grammar sampler. The few-shots parsed on their first parse.
- ✅ **WARN-7 ACQUITTED** — DEV-21 passes both scorers; no STRICT/LENIENT divergence; `clarify_rows_passed_by_question=0`.
- ✅ **WARN-R2 fix CONFIRMED IN THE FIELD** — twice, on real aborts.
- ✅ **NIT-5 CONFIRMED** — `SiegeAssistantSnapshot.cpp:614-615` says 5111, truth is **5108**. Free to fix on next touch.
- ⚠️ **NEW — the `nearest ancient ground` alias does not fix DEV-01.** The `nearest_mine` attractor survives both the `gold` deletion and a direct alias. **Class-level fix needed, not another alias.** (§3)
- ⚠️ **NEW — QA's class ranking was inverted on its most confident call.** Class (c) SELECTION-PRESERVING, rated STRONGEST, fails twice (DEV-07, DEV-16), both times by collapsing a named selection to `who:"none"`. **Highest-value lead for the next rung.** (§3)
- ⚠️ **NEW — the out-of-roster substitution attractor MOVED rather than closed** (`sapper` → `sorcerer`). The refusal rule changed the destination, not the behaviour. (§3)
- ⚠️ **NEW — the CPU abort rate is stochastic (1–2 of 5 at `deadline=10`)**, so a single 5-iteration cell is not sufficient evidence of a change in either direction.
- 📌 **Carried, untouched:** WARN-1 · WARN-2 · WARN-3 · WARN-4 · WARN-5 (the mirror is still unexecutable — nothing in this run changed that) · NIT-1..4 · ESCALATION-1.

---

## 7. MACHINE RESIDUE + RESTORE PATHS

| item | state | restore |
|---|---|---|
| Editor | **LEFT RUNNING**, PID 31460, fresh binaries, PIE **ended** | — |
| `L_Arena` | ✅ **NEVER SAVED** — mtime still `Jul 29 03:53`, **`DIRTY_COUNT: 0`** verified after PIE ended | — |
| Git | ✅ **untouched** — `git status --porcelain` byte-identical before and after; no stage, no commit, no push | — |
| Remote exec | enabled via **launch flag** (`-ini:...bRemoteExecution=True`), in-memory | **no file was edited ⇒ nothing to restore**; reverts on next editor restart |
| `Saved/Config/WindowsEditor/Engine.ini` | ✅ **NOT hand-edited** (hook-blocked lane, respected) | — |
| `EditorSettings.ini` throttle | verified `False`, **not modified by me** | — |
| Run logs | `T431_build1.log`, `T431_load_cpu.log`, `T431_prompt.log`, `T431_dev_eval2.log`, `T431_cpu_{a,b,c}_p{1,2}.log` in the session scratchpad | transient |

⚠️ **The working tree still carries three tasks' work** (TASK-428 + TASK-429 + the landed TASK-433 fix pass in `SiegeAssistantSnapshot.{h,cpp}`) — NOTE-3 stands, and **TASK-434 must not attribute the third task's edits to this wave's commit.**

---

## 8. WHAT HAPPENS NEXT

⛔ **TASK-432 IS BLOCKED. The holdout does not open.**

⇒ **Back to `gameplay-programmer` — LADDER LOOP 1 of 3.** The prompt budget is not the constraint (**33 tokens of measured slack**, more than the 18 that was budgeted for), so a fix has genuine room to work in. The three highest-value leads, in the order the measurements support:

1. **Class (c) selection-preservation — 2 rows** (DEV-07, DEV-16), both collapsing a named selection to `who:"none"`.
2. **Class (a) out-of-roster refusal — 1 row and the CATEGORICAL half of the gate** (DEV-04). The substitution must stop, not relocate.
3. **The `nearest_mine` attractor — 1 row** (DEV-01). An alias was tried and measured insufficient; this needs a class fix.

⚠️ **DEV-11 stays conceded** (coreference across clauses — the tuner declined it knowingly and I see no reason to overturn that).
⚠️ **The CPU matrix must NOT be re-run on the next loop** — tuning cannot move a decode-rate question, and §5's answer is final for this wave.

---
---

# §LOOP 2 — LADDER LOOP 2 of 3 · THE RE-RUN (build-master)

**Date:** 2026-08-03 · **Branch:** `main` · **HEAD at run:** `66c6854` (unmoved) · **No Git, no commit, no push.**
**Authorised by:** `qa/TASK-430-gate-loop2.md` — **PASS, 0 blockers**, no bar raised.
**Scope:** the TASK-428 §L2 delta only. ⛔ **No holdout opened. ⛔ CPU matrix NOT re-run** (§5 is final for this wave).

---

## L2-0. THE VERDICT, FIRST, IN ONE BLOCK

| question | answer |
|---|---|
| **Does the dev gate pass?** | ⛔ **NO. BOTH HALVES FAIL — again.** |
| **May TASK-432 spend the sealed holdout?** | ⛔ **NO. HOLDOUT 2 STAYS SEALED.** |
| Compile | ✅ **GREEN**, both lanes, zero diagnostics |
| `zoneA_chars` | ✅ **5116 — EXACTLY the required figure** |
| `zoneA_tok~=` (**MEASURED**) | ✅ **1362** vs ceiling **1389** ⇒ **27 slack. UNDER BUDGET.** |
| t0 tripwire | ✅ `zoneB_chars=68` `zoneC_chars=887` — **both exact** |
| `parse_failures` | ✅ **0** — no regression |
| Dev score | ⛔ **20/25 = 80.0 %** (gate 22/25) — **2 rows short**, up **1** from loop 1 |
| Refuse-class failures | ⛔ **1 (DEV-04)** — gate requires **ZERO**. **CATEGORICAL FAIL.** |
| Regressions | ✅ **ZERO.** All 19 loop-1 passing rows still pass. |
| ⚖️ **The clean test (DEV-07 + DEV-16)** | ⛔ **BOTH STILL FAIL, BYTE-IDENTICALLY TO LOOP 1.** |

⇒ ⚖️ **THE SUBTRACTIVE LEVER WAS MEASURED AND IT DID NOT LAND EITHER.** Loop 2 bought exactly one row — and it is
the row nobody aimed at.

---

## L2-1. STEP 0 — COMPILE. ✅ GREEN.

**Judged on the log's `Result:` line, never the exit code** (the exit code was `0` and was read as non-evidence).

```
Result: Succeeded
Total execution time: 5.71 seconds
```

**A REAL compile, not a no-op** — all three changed translation units built and both DLLs relinked:

```
[Adaptive Build] Excluded from SiegeLlama unity file: SiegeLlamaSpike.cpp
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: SiegeAssistantSnapshot.cpp, SiegeAssistantVocabulary.cpp
[1/8] Compile [x64] SiegeAssistantVocabulary.cpp
[2/8] Compile [x64] SiegeAssistantSnapshot.cpp
[4/8] Compile [x64] SiegeLlamaSpike.cpp
[6/8] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[7/8] Link [x64] UnrealEditor-SiegeLlama.dll
```

- ✅ **Zero diagnostics** — a sweep of the whole build log for `warning`/`error` returns **no lines**, under warnings-as-errors.
- ✅ All three files compiled **outside unity** (adaptive non-unity, because they are the Git working set) — the **stricter** path.
- ✅ Both DLLs stamped **04:54:39**; editor relaunched on them (**PID 28072**) and the running process confirmed the
  timestamps from inside itself. World `L_Arena`, `DIRTY_COUNT: 0`.
- 📌 It is faster than loop 1 (5.71 s vs 15.61 s) purely because UBA had a warm cache — the action list is the same shape.

**Procedure:** editor closed **gracefully** via `unreal.SystemLibrary.quit_editor()`, guarded by a dirty-package check
that **refuses to quit if anything is unsaved** (`DIRTY_BEFORE_QUIT: 0` printed before the quit). ⛔ **`Stop-Process`
was never used; nothing was force-killed; nothing was saved.** Relaunched with the **identical** launch-flag lane read
off the previous process's own command line (`-ini:…bRemoteExecution=True,…`) ⇒ **zero file edits, so no config residue.**

---

## L2-2. THE FOUR PRE-FLIGHT TRIPWIRES — ALL FOUR CLEAR

`Siege.Llama.SpikePrompt`, **no `order=` override** (so the in-code t0 tripwire is armed), model resident:

```
SPIKE_TOKENS fixture=t0 zoneA_chars=5116 zoneB_chars=68 zoneC_chars=887 zoneBC_chars=955 MaxSnapshotChars=1085 headroom=130
SPIKE_TOKENS fixture=t0 zoneA_tok~=1362 zoneB_tok~=32 zoneC_tok~=320 zoneBC_tok~=352 (<=400 cap) assembled_total_tok=1727 chars_per_token=3.56
SPIKE_TOKENS fixture=t0 assembled_prompt_chars=6151 zoneB_start_char=5163 zoneC_start_char=5231 zoneB_start_tok~=1370 zoneC_start_tok~=1402
```

| # | tripwire | required | measured | verdict |
|---|---|---|---|---|
| 1 | `zoneA_chars` | **exactly 5116** | **5116** | ✅ **EXACT — the lanes did not drift from what QA cleared** |
| 2 | `zoneA_tok~=` | ≈1352–1362 vs 1389 | **1362** | ✅ **inside the band (at its top edge). 27 tokens slack.** |
| 3 | t0 tripwire | `68` / `887` | **68 / 887** | ✅ **EXACT, tripwire never fired, no `SPIKE_WARN` about it** |
| 4 | `parse_failures` | **0** | **0** | ✅ **no regression** |

⚠️ **AN HONEST NOTE ON THE DERIVATION, BECAUSE THE DIRECTION FLIPPED.** Loop 1's derived 1371 measured **1356** —
15 tokens **high**, i.e. conservative, the safe direction §8's law wants. Loop 2's derived **1358** measured **1362** —
**4 tokens LOW, i.e. the UNSAFE direction.** It cost nothing here (27 tokens of slack), but **the derivation's error
direction is not stable and must not be treated as reliably conservative next time.** This is exactly why the rule is
to read the printed figure before scoring.

✅ **I ALSO PROVED I MEASURED THE INTENDED PROMPT, not merely one with the right byte count** — read off the rendered
Zone A dump:

- **R1** `- If the unit named is not a kind in [FORCES], answer {"ask":"unsupported"}. Never write a kind the player did not name.` — present
- **R2** `- Gold, buying and card play are the player's, never yours: {"ask":"unsupported"}.` — present, **immediately after R1, position-frozen**
- **R3** `- If the player names units, the intent is send, guard, ambush or follow, never charge, fallback or rally.` — **present**
- **R4** `- No number said: a plural = "all", a singular = 1. Never copy a count from the roster.` — present
- **R5** `- Choose a place by its noun - ancient ground, mine, castle, centre. near, nearest and far only say which one.` — **present**
- ⛔ both deleted `[notes]` sentences: **0 occurrences** · ⛔ the retired selection rule (`never dropped`): **0 occurrences**
- ✅ **`nearest_mine` occurrences in Zone A = 2** (the claimed 3 → 2, confirmed in the rendered text)
- ✅ the reframed exemplar renders as `order: send werewolves to the middle`

⇒ **Every mechanism loop 2 shipped was physically in the prompt the model read. Nothing below is a "the edit didn't
apply" artifact.**

---

## L2-3. ⚖️ THE DEV GATE. **NOT MET, ON BOTH HALVES.**

`Siege.Llama.SpikeEval tier=full gpu=0`, model pinned to `dev=0/Vulkan0/GPU desc="NVIDIA GeForce RTX 5070 Laptop GPU"`.
⛔ **`holdout=` was never passed.** The harness confirms: `SPIKE_RUN: no holdout= given, so ONLY the dev split was scored.`

### Reported exactly as printed

```
SPIKE_EVAL_SCORE split=dev rows=25 PRIMARY(lenient)=20/25 = 80.0% | STRICT=20/25 = 80.0%
                 | parse_failures=0 | clarify_rows_passed_by_question=0 | assertions_compared=70
SPIKE_EVAL_SCORE split=dev deferred-trigger agreement (NOT part of bar #5's four fields) = 1/1
SPIKE_EVAL_SCORE split=dev LENIENCY FLOOR: a degenerate model that answers {"ask":...} to EVERY sentence
                 would score 12/25 = 48.0% under the PRIMARY rule (it fails every Execute row).
SPIKE_EVAL_SCORE split=dev 6 row(s) assert at most ONE field and therefore test less than the score implies:
                 DEV-04 DEV-06 DEV-08 DEV-09 DEV-10 DEV-25
SPIKE_EVAL_SCORE split=dev 1 row(s) assert NOTHING AT ALL and pass on any parseable output: DEV-08
```

### The two halves

| half | requirement | measured | verdict |
|---|---|---|---|
| **(i)** | **≥ 88 % (22/25)** | **20/25 = 80.0 %** | ⛔ **FAIL — 2 rows short** |
| **(ii)** | **ZERO Refuse-class failures** | **1 Refuse row emitted a live executable order** | ⛔ **CATEGORICAL FAIL** |

**Trajectory: baseline 18 → loop 1 19 → loop 2 20.** Each wave has moved the number by **exactly one row**.
Loop 2's prediction was 20–22; it landed at the **bottom** of that band.

### ⛔ HALF (ii) — THE REFUSE CLASS, CALLED OUT SEPARATELY, ALL THREE ROWS

| row | sentence | expect | got | verdict | emitted |
|---|---|---|---|---|---|
| **DEV-04** | `send the catapults at the enemy base` | Refuse | **`command`** | ⛔ **FAIL — LIVE ORDER** | `{"intent":"send","who":[{"kind":"sapper","n":1}],"where":"enemy_castle","when":"now"}` |
| DEV-09 | `play a knight` | Refuse | `question` | ✅ **PASS** | `{"ask":"unsupported"}` |
| DEV-10 | `buy a miner` | Refuse | `question` | ✅ **PASS** | `{"ask":"unsupported"}` |

✅ **JONATHAN'S STANDING RULING IS NOT BROKEN.** The two economy/card-play rows both hold, **byte-identically to
loop 1**. R2 stayed byte- and position-frozen and it stayed the one class that works.

⛔ **But the gate says ZERO, and zero is not 1. Even a 22 would not have opened the holdout with DEV-04 in this state.**

### ⚖️ THE FLIPPED-ROW LIST — WHICH IS THE REPORT, NOT THE TOTAL

I diffed all 25 rows against loop 1 **mechanically** (raw output string compared byte-for-byte), not by eye.

| | rows |
|---|---|
| ✅ **FLIPPED FAIL → PASS** | **`DEV-11` — and nothing else.** |
| ⛔ **STILL FAILING** | **`DEV-01`, `DEV-04`, `DEV-07`, `DEV-16`, `DEV-20`** |
| ✅ **REGRESSIONS** | **NONE. Zero.** All 19 loop-1 passing rows still pass. |

⚠️⚠️ **AND THE FACT THAT DOMINATES EVERYTHING ELSE IN THIS REPORT: 23 OF THE 25 ROWS RETURNED OUTPUT THAT IS
BYTE-IDENTICAL TO LOOP 1.** Only two rows produced a different string at all — `DEV-11` and `DEV-04`. Five rule lines
were rewritten, one rule was added, two `[notes]` lines were deleted and an exemplar was reframed, and **92 % of the
corpus did not move by a single character.**

### ⚠️ THE READING RULE, APPLIED ROW BY ROW AS QA WEIGHTED THEM

**⚖️ `DEV-07` + `DEV-16` — THE CLEAN TEST OF THE SUBTRACTION THESIS. BOTH STILL FAIL, BYTE-IDENTICALLY.**

```
DEV-07  charge with the footmen          {"intent":"charge","who":"none","where":"none","when":"now"}         <- identical to loop 1
DEV-16  infantry back to our castle now  {"intent":"fallback","who":"none","where":"own_castle","when":"now"} <- identical to loop 1
```

⚠️ **This is the sharpest negative result of the wave, and DEV-07 is the row that makes it airtight:**

- R3's antecedent (*"if the player names units"*) is **unambiguously satisfied** — `the footmen` is a listed kind's plain plural;
- **few-shot #6 uses the identical noun phrase** (`i want the footmen to rush` → `send`) and is untouched;
- the competing general rule was **deleted**, not out-shouted;
- R3 **names `charge` explicitly as forbidden** and was verified present in the rendered rules block;
- ⇒ **and the model emitted `charge` with `who:"none"` anyway, to the byte.**

⇒ ⚖️ **QA'S §L2b "HONEST LIMIT" IS THE ONE THAT FIRED.** The four `who="none"` lines are still printed, and the model
latches the intent off the **verb** before it ever evaluates R3's antecedent. **A prompt cannot force evaluation
order — and this run is the measurement that says so.** The gate mechanism was structurally correct, was physically
present, and produced **zero** change in the emitted bytes on both rows it was designed for.

**`DEV-01` — uninformative, as QA said. Still fails, byte-identically** (`"where":"nearest_mine"`). R5 + the attractor
3→2 did not move it. Per §L9c its flip would have proved nothing; **its non-flip likewise proves little**, and I am
not resting anything on it.

**⛔ `DEV-04` — THE CATEGORICAL GATE. STILL A LIVE ORDER — and the ONE row whose output genuinely changed.**

This is the only place loop 2 demonstrably altered model behaviour, and it is worth the detail:

| run | emitted `who` | is it a substring of a few-shot output? |
|---|---|---|
| baseline | `{"kind":"sapper","n":1}` | no — semantic near-neighbour |
| loop 1 | `{"kind":"sorcerer","n":1}` | ✅ **yes — character-for-character in few-shot #1** (QA §L3) |
| **loop 2** | **`{"kind":"sapper","n":1}`** | ⛔ **no — `{"kind":"sapper"` occurs 0 times in Zone A** |

✅ **R1's new second sentence DID what it was written to do: the slot-fill route is measurably closed.** The verbatim
exemplar substring is gone from the output, and I verified `{"kind":"sapper"` appears **nowhere** in the rendered
prompt (`sapper` appears exactly once, in an alias row — `sapper <- bomber, bombers, demolition, sappers` — and
`catapults` is not one of those aliases).

⛔ **And the row still failed, by reverting to the baseline's semantic substitution.** R1's antecedent was **true**
(`catapults` is not a kind in `[FORCES]`), the rule commands `{"ask":"unsupported"}`, and the model issued a command
instead. ⇒ **Two independent routes into this row have now each been closed on separate loops, and the refusal never
happened. The attractor is the substitution BEHAVIOUR, not any particular destination.**

📌 **Follow-up worth a task, noticed in passing:** the roster prints `- sapper: 2 total, 0 orderable, 2 followable`,
so the emitted live order names a kind the board itself says is **not orderable**. That is a downstream-validation gap
(a command layer should reject it regardless of what the model says), not a prompt defect — **reporting it, not
acting on it.**

**`DEV-20` — a pre-registered falsification FIRED.** The programmer wrote: *"If DEV-20 comes back `n:1` again, the
finding is that clause order does not steer this model, and no further rewording of this line is worth a loop."*
**It came back `n:1`, byte-identical.** ⇒ **That finding is now measured. R4's clause reorder does not steer this
model, and the line must not be reworded again.**

⚠️ **AND R4 HIT A ROW IT WAS NOT AIMED AT.** The single flip, `DEV-11`, is attributable to R4's **new antecedent**
(*"No number said:"*), which blocks the `"all"` default when a count *was* said:

```
DEV-11  wait until i have 2 more knights then send them mid
  loop 1: ...{"kind":"knight","n":"all"}...  FAIL (counts=BAD)
  loop 2: ...{"kind":"knight","n":2}...      PASS (all four fields ok)
```

⇒ ⚠️ **`DEV-11` WAS THE KNOWINGLY CONCEDED ROW.** It appears in no fix plan; the programmer's §L2.9 table lists the
five targeted rows as DEV-16, 07, 04, 01, 20. **None of the five targeted rows moved. The only row that flipped is the
one that was written off.**

---

## L2-4. ⚠️ THE PRE-REGISTERED DIAGNOSES — WHICH FIRED, WHICH DID NOT

Honoured as written, before the run, so none of this is fitted afterwards.

| pre-registered | condition | result |
|---|---|---|
| ⚖️ **WARN-L2-1** — *if `DEV-09` or `DEV-10` flips to a command, the cause is **R3**, not R2* | did either flip? | ✅ **DID NOT FIRE.** Both still `question`, both byte-identical. **R3 did not poach the economy rows.** ⇒ QA's §L7 three-protection argument (R2 byte/position-frozen · few-shot #5 untouched · the deletion off their path) is **vindicated by measurement**. ⛔ **R2 was not touched and must stay frozen.** |
| ⚠️ **WARN-L2-2** — *if `clarify_rows_passed_by_question` ≠ 0, or STRICT < LENIENT, inspect **DEV-06** first, **DEV-22** second* | either fingerprint? | ✅ **DID NOT FIRE.** `clarify_rows_passed_by_question=0`; **STRICT = LENIENT = 20/25**, no divergence on any row. **DEV-06 and DEV-22 both PASS as commands, byte-identical to loop 1.** ⇒ **R1's sharpened `[FORCES]` antecedent did NOT over-refire.** |
| ⚠️ **WARN-L2-3** — *if an `own_castle`/`enemy_castle` row regresses, R5's enumeration is the first suspect* | any place regression? | ✅ **DID NOT FIRE.** Every place-asserting row passes: DEV-03/12/13/17/21 (`enemy_castle`), DEV-16's `own_castle` field, DEV-18 (`ancient_ground_near`), DEV-05/24 (`ancient_ground_far`), **DEV-23 (`nearest_mine`, the row R5 was worded to protect)**, DEV-22 (`"none"` for "river"). **R5 cost nothing.** |
| ⚠️ **programmer, §L2.6** — *if DEV-20 returns `n:1`, clause order does not steer this model* | did it? | ⛔ **FIRED.** See §L2-3. |

⇒ ⚖️ **Every regression QA feared was measured and none happened. The loop-2 delta is SAFE. It is simply INERT on the
rows it was aimed at.** Those are two separate findings and both are true.

**NOTE-5 check:** `ROW_DID_NOT_COMPLETE` count = **0** across all 25 rows. Every failure is a genuine comprehension
failure, not a decode cut. **`llama_sampler_init_grammar returned NULL`: 0 occurrences** ⇒ the grammar sampler was
non-NULL for the whole run, and `parse_failures=0` is a **parse**, not a reading.

---

## L2-5. ⚖️⚖️ THE QUESTION §L9c PRE-REGISTERED: DID THIS RUN PRODUCE A NEW **COUNTABLE MECHANISM**?

QA's bar: a loop 3 is refused unless this run yields a **countable fact read off the artifacts**, of the kind loop 1
produced twice — **not a hypothesis about the model.**

**It produced three countable facts. I am listing them, and then I am going to say the uncomfortable thing about them.**

1. **23 of 25 rows are byte-identical to loop 1.** Countable, mechanical, verified by string comparison. A 5-rule
   rewrite + 2 deletions + an exemplar reframe changed the emitted text on **two** rows.
2. **The slot-fill route into DEV-04 is closed, and DEV-04 still fails.** `{"kind":"sorcerer","n":1}` is a
   character-for-character substring of few-shot #1's output and is **gone**; `{"kind":"sapper"` occurs **0 times** in
   the rendered Zone A. The substitution destination sequence across three runs is **sapper → sorcerer → sapper**.
3. **R3 was present, its antecedent was satisfied, its own exemplar used the identical noun phrase, its competitor was
   deleted — and DEV-07's output did not change by one byte.**

⚠️⚠️ **AND HERE IS THE HONEST READING, WHICH IS THE POINT OF HAVING A PRE-REGISTERED BAR AT ALL: none of the three
names an edit.** Every one of them is a **negative** result — a route measured closed with no behavioural change, or a
mechanism measured present with no effect. §L9c's bar exists to separate engineering from lottery tickets, and **a
countable fact that tells you a lever is inert does not fund another pull of that lever; it retires it.**

⇒ ⛔ **MY READING: THE §L9c CONDITION FOR A LOOP 3 IS NOT MET.** I hold the data and I am not going to dress a negative
result up as a lead. **QA owns this ruling — but it should rule knowing that I found countable facts and that every one
of them points at the exit.**

### 🧑 THE FINDING, WRITTEN PLAINLY FOR JONATHAN, BECAUSE THIS IS WHAT THE DATA SAYS

> **Prompt-level tuning on this model tops out at 20/25 against a gate of 22.**
>
> Three waves, three measurements: **18 → 19 → 20.** Each wave moved **exactly one row**. The first spent **+797
> chars** (18.5 % of Zone A) to do it; the second spent **+8 chars** to do it — **so the cost per row is not the
> constraint, and neither is the token budget** (27 tokens still unspent). The additive lever was declared dead on
> three measurements. **The subtractive lever has now been measured too, and it moved one row that nobody was aiming
> at while leaving 23 of 25 outputs byte-identical.**
>
> ⚠️ **The categorical half of the gate is the harder fact.** DEV-04 must refuse and it has never refused — across a
> rule that named the behaviour, a rule that named the list, a purpose-built exemplar, and a sentence that forbids
> writing an unnamed kind. Two distinct routes into that row have each been measured shut, and the row still emits a
> live order. **The gate cannot be passed by a number alone; this row has to stop, and four prompt-level attempts have
> not stopped it.**
>
> **The next rung is a bigger model. You reserved that decision for yourself, and this is the evidence you reserved it
> for.** ⚠️ **This is a result, not a defeat** — it is bought with measured mechanisms rather than impressions, and it
> tells you exactly what the reserved rung is for. The prompt work was not wasted either: it is **safe** (zero
> regressions across two loops), it **fixed the gold/card-play refusal** that violated your standing ruling, and it
> **closed the exemplar slot-fill route**. It simply cannot reach 22.

⚠️ **AND THE CPU ANSWER TRAVELS WITH IT, UNCHANGED:** the CPU fallback **works and is merely slow** — a tunable
deadline artifact, not a wall, **not** a thread bug — correct on correctness, **~1.6× outside bar #2 on latency**.
⛔ **§5 was NOT re-run** (board ruling): the only load I performed this pass was `Siege.Llama.SpikeLoad tier=full gpu=0`
to make the tokenizer resident, which is a load and not a bench cell — **it produced no ms/token figure and no §5
number is restated or revised.**

---

## L2-6. ⛔ THE HOLDOUT SEAL — INTACT, VERIFIED NOT ASSERTED

- **No `holdout=` argument was passed to any command in this pass.**
- The harness printed its own confirmation: `SPIKE_RUN: no holdout= given, so ONLY the dev split was scored.`
- Swept every log from this pass: **0 `split=HOLDOUT` lines, 0 `HOLD-` row ids.**
- Neither `assistant_eval_holdout.csv` nor `assistant_eval_holdout2.csv` was opened, by any command or by me.
- ⚠️ **NOTE-4 honoured: no per-row line from any holdout appears in this artifact.** Every row quoted is a `DEV-` row.

⇒ ✅ **TASK-432's one-shot is UNSPENT and remains available — but it is NOT authorised to be spent.**

---

## L2-7. MACHINE RESIDUE + RESTORE PATHS

| item | state | restore |
|---|---|---|
| Editor | **LEFT RUNNING**, **PID 28072**, fresh 04:54:39 binaries, no PIE entered this pass | — |
| `L_Arena` | ✅ **NEVER SAVED** — mtime still `2026-07-29 03:53:38`, `DIRTY_COUNT: 0` before the close **and** after the run | — |
| Editor close | `quit_editor()` behind a dirty-guard that refuses when unsaved. ⛔ **No force-kill, no `Stop-Process`** | — |
| Git | ✅ **untouched** — `git status --porcelain` + `HEAD` **byte-identical** before and after; no stage, no commit, no push. HEAD `66c6854` | — |
| Remote exec | launch flag only (`-ini:…bRemoteExecution=True`), in-memory | **no file edited ⇒ nothing to restore** |
| `Saved/Config/WindowsEditor/Engine.ini` | ✅ **NOT hand-edited** (hook-blocked lane, respected) | — |
| CPU matrix | ⛔ **NOT RE-RUN** | — |
| Run logs | `T431L2_build.log`, `T431L2_prompt.log`, `T431L2_load.log`, `T431L2_prompt2.log`, `T431L2_dev_eval.log` in the session scratchpad | transient |

⚠️ **NOTE-3 STANDS** — the working tree still carries **three** tasks' work (TASK-428 + TASK-429 + the landed TASK-433
fix pass). **TASK-434 must not attribute the third task's edits to this wave's commit.**

---

## L2-8. WHAT HAPPENS NEXT

⛔ **THE DEV GATE FAILS ON BOTH HALVES. TASK-432 IS BLOCKED AND MUST NOT SPEND THE SEALED HOLDOUT.**

⚠️ **The route is NOT automatically loop 3, and on my reading it is not loop 3 at all** (§L2-5): this run produced
countable facts, but every one of them retires the lever rather than aiming it. **Per QA's §L9c the escalation goes to
Jonathan with the §L9b finding — the next rung is his and he reserved it.**

**Carried forward for whoever picks this up:**
- ⛔ **R2 stays byte- and position-frozen.** It is the one class that works and it survived loop 2 untouched.
- ⛔ **R4's counts line must not be reworded again** — its own pre-registered falsification fired.
- ✅ **The loop-2 delta is safe to keep**: zero regressions, +1 row, ~27 tokens still unspent. **Nothing here needs reverting.**
- 📌 **NEW, worth a task:** a live order was emitted naming a kind the roster reports as `0 orderable` — the command
  layer should reject that independently of the model.
- 📌 **WARN-5 / NIT-L2-3 unchanged:** `zoneA_chars=5116` proves the **spike** lane only; the shipped `BuildZoneA` byte
  count is still a reading-level claim. The automation test QA recommends for TASK-423 would close it.
