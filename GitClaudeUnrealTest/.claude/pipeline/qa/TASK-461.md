# QA Report — TASK-461 (gate on TASK-459)

**Verdict: ✅ PASS — 0 BLOCKERS · 2 WARN · 2 NIT**
Scope: `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`, function
`FSiegeLlamaWorker::RunRequestUnguarded`, two `FString::Printf` arguments + 26 comment lines.
**Not** a re-review of the subsystem (that is `qa/TASK-446.md` §11).

## 0. ⛔ WHAT I DID NOT RUN — said first

**Nothing was compiled and nothing executed.** No UBT/UHT, no linker, no editor, no PIE, no MCP.
⛔ **I do not say "the tests pass"** — nothing in this repo has ever been built or run. Every
statement below is a statement about **source**, read with `Read` (raw bytes), located with `Grep`.

⚠️ **AND I HAVE NO SHELL IN THIS ROLE — SO TWO OF 459's PROOFS WERE RE-DERIVED, NOT RE-RUN** (see §5).
I could not execute `git`, `stat` or a byte-count. **Where 459 cited an mtime or a `git diff`, I
verified the underlying PROPERTY at the file instead of re-running its instrument, and I say which.**

---

## 1. ⭐ THE RULING ASKED FOR — THE SECOND SITE (§15 DEPARTURE): **RATIFIED**

**Verified from the code, not from the handoff.** Both sites are the identical defect, and both are
now correct:

| # | site (by symbol, §18c) | guard | printed argument NOW | verified |
|---|---|---|---|---|
| 1 | `RunRequestUnguarded` → prefill loop, bottom-of-body deadline branch | `:1550` `if (FPlatformTime::Seconds() > HardDeadlineSeconds)` | `:1572` `FPlatformTime::Seconds() - StartSeconds` | ✅ |
| 2 | `RunRequestUnguarded` → decode loop, per-token deadline branch | `:1707` `if (Now > HardDeadlineSeconds)` | `:1725` `Now - StartSeconds` | ✅ |

- ✅ **SAME DEFECT, CONFIRMED ON ALL FOUR AXES:** same constant (`USiegeLlamaSubsystem::HardTimeoutSeconds`),
  same field (`"%.1fs elapsed"` — `:1571` and `:1724`, byte-compared), same function, same failure
  mode (**a claim about the configuration standing where the reader expects a claim about the world**).
- ⭐ **AND THE MEASURED CASE FOR THE DEPARTURE HOLDS — I CHECKED IT AT TASK-413's TABLE, NOT AT THE
  HANDOFF'S SUMMARY OF IT** (`handoffs/TASK-413-buildmaster.md:944-953`):

  | tier | worst TTFT | worst wall | aborts |
  |---|---|---|---|
  | full | 434.1 ms | 2937.5 ms | 0/5 |
  | partial | 1705.6 ms | **8926.3 ms of 10000** | 0/5 |
  | cpu | 2913.8 ms | 10000.6 ms | **2/5** |

  ⇒ **Worst TTFT on ANY tier is 2913.8 ms — 29 % of the ceiling — and TTFT is an UPPER BOUND on
  prefill** (it is `FPlatformTime::Seconds() - StartSeconds` sampled at `TokenIndex == 0`, `:1662`,
  so it contains tokenize + prefill + the first `llama_sampler_sample`). **The prefill branch is
  therefore barely reachable, and the bound is conservative in the safe direction.** Meanwhile
  partial's worst run finished **1074 ms under** the ceiling with the deadline armed on total wall —
  **one slow token from the decode branch**, on a tier where `abort_callback` enforces nothing.
- ⛔ **THEREFORE FIXING ONLY THE CITED SITE WOULD HAVE LEFT TASK-447 QUOTING A CONSTANT ANYWAY** —
  this task's own failure mode, reached by a task that reported success. **The departure is not a
  liberty; declining it would have been the defect.**
- ⚖️ **AND IT MEETS §15's RATIFICATION TEST — THE MECHANISM IS CHECKABLE WITHOUT TRUSTING THE
  REFUSER:** two format strings and one published table. **It also touches none of the four things
  the fence enumerates** (flag · guard · placement · `HardTimeoutSeconds`) — verified individually
  in §3. **Separability was offered and is not needed.**

---

## 2. ⭐ §23 — THE **BASE**, CHECKED AT THE SYMBOL. CONFIRMED CORRECT AT BOTH SITES

**This is the check that would have passed a wrong fix, and it passes correctly.**

- ✅ **THE DEADLINE'S OWN BASE:** `:1454` `const double HardDeadlineSeconds = StartSeconds + USiegeLlamaSubsystem::HardTimeoutSeconds;`
  ⇒ **`printed − 10.0` IS the overshoot, directly, with no conversion.**
- ✅ **ONE `StartSeconds`, DECLARED ONCE, NO SHADOW:** `:1292` `const double StartSeconds = FPlatformTime::Seconds();`
  is the **only** declaration in the function. **Proof that both sites are in that same scope without
  reading offsets:** `:1735` evaluates `DecodeStart` (declared `:1643`) and `StartSeconds` (declared
  `:1292`) **in one expression**, so a single contiguous scope spans both sites.
- ⛔ **THE TWO WRONG BASES WERE GENUINELY IN REACH AND NEITHER WAS TAKEN:** `PrefillStart` (`:1469`)
  and `DecodeStart` (`:1643`) are both live at both sites; **both vary**; **both are uncomparable
  with the ceiling** because they exclude time the deadline was already counting. **The shipped
  expressions subtract `StartSeconds` at both sites.** ✅
- ✅ **SITE 2 TAKES NO NEW CLOCK READ.** `:1706` `const double Now = FPlatformTime::Seconds();`
  is the value tested at `:1707` and the value printed at `:1725` — **the printed number IS the
  number that tripped the guard.**
- ✅ **SITE 1's FRESH READ IS CORRECT AND ITS ERROR IS UNMEASURABLE HERE:** the gap between the test
  at `:1550` and the read at `:1572` is one branch entry and two assignments — **sub-microsecond,
  four orders of magnitude under the `%.1f` field's 0.1 s resolution.**
- ✅ **TYPE/VARARG CONTRACT UNCHANGED:** `FPlatformTime::Seconds()` → `double`; `StartSeconds`,
  `Now` → `double`; the removed `HardTimeoutSeconds` was `static constexpr double`. **`double` for
  `double` into an unmodified `%.1f`, format string still a literal** ⇒ the `TCheckedFormatString`
  path is untouched and **no new compile risk is introduced by these two arguments.**

---

## 3. SCOPE-FENCE CHECK — ALL HELD (verified by symbol at the current file)

| fence | verified at | result |
|---|---|---|
| `HardTimeoutSeconds` still verbatim `10.0` | `SiegeLlamaSubsystem.h:161` `static constexpr double HardTimeoutSeconds = 10.0;` | ✅ |
| header claim not softened | `.h:133-147` BACKSTOP paragraph intact, **both non-claims present** (`:145-147` no mid-graph cut on GPU tiers; `:149-153` the clock starts on the worker) | ✅ |
| guard **conditions** byte-intact | `:1550` and `:1707`, unchanged text | ✅ |
| guard **placement** (§21, load-bearing) | prefill test still **BELOW** `Offset += ChunkTokens` (`:1514`) and `CheckSoftTimeout` (`:1518`) ⇒ **bottom-of-body**; cancel/stop test still at the **top** (`:1486`) | ✅ |
| `bAborted` at both exits | `:1556` and `:1709`, `bAborted = true;` alone | ✅ |
| post-prefill guard | `:1594` `if (bCancelled \|\| bDecodeFailed \|\| bAborted)` | ✅ |
| `%.1f` deliberately **not** widened | `:1571`, `:1724` | ✅ — **ruled on in §4** |
| `SiegeLlamaSpike.cpp` (🔒 §16) not "fixed" | `:3144-3145` **still** feeds `Options.HardTimeoutSeconds` into `DEADLINE %.1fs elapsed` | ✅ — correctly left alone |

**On the 26 comment lines against a *"change nothing else"* spec — RULED PROPORTIONATE.** The fence
enumerates **four behavioural items**; comments are not among them and are not behaviour. **13 lines
per site** (`:1557-1569`, `:1710-1722`) matches this file's ambient density (`:1436-1452`,
`:1520-1549`, `:1579-1593`) — and they encode **the one thing a later "tidy-up" would destroy: why
the base is `StartSeconds` and not the nearer variable.** ⚖️ **That is the §23 trap written into the
code at the exact line where it would be sprung.** ✅ **Keep them** (subject to WARN-1's precision fix).

📌 **INDEPENDENT CORROBORATION OF THE DIFF ARITHMETIC, WITHOUT GIT:** the two comment blocks are
**13 + 13 = 26 lines** and the two arguments are **2 lines** ⇒ **28 insertions / 2 deletions**, which
is exactly what 459's `git diff --no-index` reported. **The file itself accounts for every line
claimed** (§18a: check the itemisation, never the number).

---

## 4. THE DECLARED RESIDUE — `%.1f` NOT WIDENED: **THE RESTRAINT WAS RIGHT**

`%.1f` floors at ~50 ms, so a sub-50 ms overshoot still renders `10.0s`. ✅ **Ratified as declared,
for three reasons and not merely because the spec said one argument:**

1. ⚖️ **THE TWO OUTCOMES ARE NOT THE SAME CLAIM EVEN WHEN THE GLYPHS MATCH.** Before: *a constant
   that ignores the world.* After: *a measurement that rounds to the ceiling.* **`10.0s` now means
   "the overshoot was under 50 ms" — which is information the line could not previously carry.**
2. ✅ **A FIX THAT QUIETLY WIDENS ITS OWN SCOPE IS HARDER TO REVIEW THAN ONE THAT STATES ITS
   RESIDUE** (§23's own wording). Declaring it put the decision where the manager can take it.
3. ⚠️ **AND THE FLOOR IS UNLIKELY TO BIND**, because the overshoot is **one `llama_decode` call** —
   one token in decode, up to one `n_batch` slice in prefill. **⛔ I am NOT converting TASK-413's
   means into a per-token figure and calling it a bound** (that is the proxy-as-property trap
   §18a names, and 459 correctly declined it too). **The real overshoot is UNMEASURED until
   TASK-447 takes it, and that is the honest state.**

⇒ **If the manager later judges 0.1 s too coarse, it is a one-character follow-up. It is not a
condition of this PASS.**

---

## 5. ⛔ THE CPU-PATH CARRY — CONFIRMED AT THE CODE. **TASK-447 MUST NOT READ SILENCE AS "NO TIMEOUT"**

**Traced end to end, because this changes what 447 is allowed to conclude:**

- `:955-956` `ContextParams.abort_callback = &SiegeLlamaPrivate::AbortThunk;` ⇒ `:584-587`
  `AbortThunk` → `:439-447` `ShouldAbortNow()`, which returns true on
  `NowMicroseconds() > AbortDeadlineUsec` — **the deadline, armed at `:1455` from the same
  `HardDeadlineSeconds`.**
- `llama.h:382-385` (quoted verbatim in the header at `.h:126-128`): abort_callback *"currently works
  only with CPU execution."*

| tier | what fires at the ceiling | what is logged |
|---|---|---|
| **cpu** | `abort_callback` cuts **mid-graph** ⇒ `llama_decode` returns **2** | ⛔ the `:1696-1699` `DECODE llama_decode=%d …` branch — **NO elapsed field.** The `HARD TIMEOUT` line **never prints** |
| **partial / full** | callback enforces nothing; the **loop test** is the enforcement | ✅ the `:1724` line this task made real |

⛔ **CONFIRMED: on `tier=cpu` the absence of a `HARD TIMEOUT` line is NOT evidence that no timeout
occurred** — it is evidence the abort came the other way. **§21's "absence of a log line is not
evidence the guard works", aimed at the instrument.** ⇒ **The elapsed number this task makes real is
the GPU-tier number, and partial — worst wall 8926 ms of 10000 — is the tier closest to firing it.**

---

## 6. §22 SWEEP — **MINE, NOT 459's.** RESULT: **NOTHING FURTHER**

**A sweep that finds nothing more is a RESULT and is reported as one.** I searched the **shape**
(`elapsed|ceiling|%.1fs|TimeoutSeconds|DEADLINE`) across the **whole plugin**, and `elapsed` across
the whole game module — not the cited line.

| site | verdict |
|---|---|
| `SiegeLlamaSubsystem.cpp:1571-1572` prefill | ⛔ was the defect — **FIXED, verified** |
| `SiegeLlamaSubsystem.cpp:1724-1725` decode | ⛔ was the defect — **FIXED, verified, cited by nobody** |
| `SiegeLlamaSubsystem.cpp:884-885` cpu-tier load warning — *"at the %.1fs ceiling"* | ✅ **CORRECT — the field is NAMED as the ceiling.** Do not "fix" |
| `SiegeLlamaSubsystem.cpp:993/1004` — `deadline=%.1fs` config dump | ✅ **CORRECT — a configuration dump** |
| `SiegeLlamaSubsystem.cpp:1807-1809` `CheckSoftTimeout` — *"SOFT timeout of %.1fs passed … continues to the %.1fs hard ceiling"* | ✅ **CORRECT — both arguments are thresholds and both are labelled as thresholds.** ⚠️ **Nearest neighbour of the shape; do not "fix" it** |
| 🔒 `SiegeLlamaSpike.cpp:3144-3145` `DEADLINE %.1fs elapsed` ← `Options.HardTimeoutSeconds` | ⚠️ **SAME SHAPE, STILL PRESENT — §16-pinned, correctly untouched.** Carried to TASK-447, **not a defect of this task** |
| 🔒 `SiegeLlamaSpike.cpp:3147-3148` *"hard timeout of %.1fs hit"* · `:4251-4252` non-default-deadline warning | ✅ **CORRECT — ceiling named as ceiling** |
| `Barracks.cpp:163-164` *"Lifetime (%.0f s) elapsed"* ← `LifetimeSeconds` (game lane) | ✅ **NOT the shape — the field is named `Lifetime`, the value IS the configured lifetime, and an engine timer fired at exactly it.** Swept and cleared so a later reader does not "fix" it |

⇒ ✅ **After TASK-459 there is no remaining site in the SHIPPED subsystem that prints a configured
bound into a measured field. The only surviving instance in the repo is the §16-pinned spike.**

---

## 7. Findings

- **[WARN-1] `SiegeLlamaSubsystem.cpp:1712-1715` — the new comment states TASK-413's *TTFT* and
  *total-wall* figures as *"prefill"* and *"DECODE wall"*. The conclusion is right; the labels name a
  proxy as the property (§18a), and this is the text TASK-447 will quote.**
  The comment reads *"Measured prefill is 0.4-2.9 s worst-case across all three tiers (TASK-413)"* —
  but `413:944-953` publishes **`mean_ttft_ms` 417.3 / 1655.4 / 2716.1** and **worst TTFT 434.1 /
  1705.6 / 2913.8**; **413 never published a prefill figure at all** (the SUMMARY line carries only
  `ttft` and `wall`). Likewise *"the partial tier's worst DECODE wall was 8926 ms"* — **8926.3 is
  `WORST_wall_ms`, the whole request**, not a decode-only measure. ⚖️ **NEITHER ERROR DAMAGES THE
  ARGUMENT, AND BOTH ERR SAFE:** TTFT **upper-bounds** prefill, so *"the prefill ceiling is barely
  reachable"* is if anything understated; and **total wall is the RIGHT quantity to compare against a
  ceiling armed on `StartSeconds`** — so the comparison in the second clause is more correct than its
  label. ⚠️ **But this is §23's own lesson landing on §23's own comment: a figure's base is part of
  the figure.** **Fix (documentation, non-blocking):** say *"worst TTFT — an upper bound on prefill —
  was 434 / 1706 / 2914 ms"* and *"partial's worst total wall was 8926 ms of 10000."*
  ⛔ **TASK-447 MUST QUOTE THE CORRECTED FORM, NOT THE COMMENT.**

- **[WARN-2] `SiegeLlamaSubsystem.cpp:1696-1699` — the decode loop's `llama_decode=2` detail string
  omits the three-cause disambiguation its prefill twin carries, and on `tier=cpu` this is the branch
  TASK-447 will actually land in.** The prefill version (`:1509`) spells it out: *"a cancel, a
  shutdown OR the hard deadline landing mid-graph. All three feed `ShouldAbortNow()`, so the code
  alone does not say which."* The decode version says only *"code 2 is ABORTED, so the JSON is
  TRUNCATED, not wrong."* ⚠️ **Per §5, code 2 with no cancel outstanding is the CPU tier's normal
  deadline signature** — so 447 reads a line that does not name the deadline, in a run that timed
  out, with no elapsed field anywhere. ⛔ **PRE-EXISTING, NOT INTRODUCED BY TASK-459, AND FIXING IT
  HERE WOULD HAVE BREACHED THE SCOPE FENCE — 459 was right not to touch it.** **Disambiguation rule
  for 447, derivable at the code:** `bCancelled = bAborted && bCancelRequested` (`:1695`), so
  **code 2 + `bCancelled` false ⇒ deadline or shutdown, never a player cancel.** Manager's call
  whether to raise a sibling task.

- **[NIT-1] `.claude/pipeline/TASKBOARD.md` TASK-459 `delivered:` repeats WARN-1's two mislabels**
  (*"measured prefill is 417-2914 ms"*, *"partial's worst DECODE wall"*). **Board text outlives
  handoffs.** Correct it when the board is next touched.

- **[NIT-2] The board's spec cites `SiegeLlamaSubsystem.cpp:1557-1559`, which is now the comment
  block, not the code.** ✅ **459 flagged this itself and worked by symbol (§18c).** Recorded so a
  later reader does not chase the offset. **No action.**

---

## 8. ⚠️ WHAT THIS PASS DOES NOT CERTIFY — stated, not buried

1. ⛔ **THAT IT COMPILES.** Unverified by me and unverifiable in this role. **TASK-447 is the gate.**
   What I *can* say: both substitutions are `double`→`double` into an unmodified `%.1f`, both symbols
   are in scope at their sites, every added line is `//` (no `*/` terminator trap), and the format
   strings are unchanged literals. **I did NOT re-run 459's brace/paren counts** (215/215, 938/938) —
   **unverified, and not load-bearing for this verdict.**
2. ⛔ **THE HEADER-MTIME CLAIM (13:25 vs 13:49) IS NOT RE-RUN — I have no shell.** ✅ **I verified
   the stronger property instead: the header's CONTENT still matches what `qa/TASK-446.md` §11
   ratified** — `10.0` verbatim at `:161`, BACKSTOP intact at `:133-147`, both non-claims present.
   ⚖️ **An mtime is evidence about a filesystem; the content is the claim.**
3. ⛔ **"`SiegeLlamaSpike.cpp` UNMODIFIED" IS NOT RE-RUN EITHER.** I confirmed the property that
   matters — **the §16-pinned file still carries the unfixed shape at `:3144-3145`, i.e. 459 applied
   no fix there.** I cannot certify byte-identity of 4700 lines without `git`; **build-master can,
   and it is tracked, so plain `git diff` is valid on that file.**
4. ⛔ **THAT EITHER LINE EVER PRINTS.** Requires a run crossing 10 s that exits via the loop test
   rather than `abort_callback`. **Unobserved. Do not score it by silence** (§21).
5. ⛔ **THE ACTUAL OVERSHOOT.** 459 makes it *measurable*; it does not *measure* it. **The first real
   reading is TASK-447's, and it should be quoted as `printed − 10.0`.**

---

## Notes for build-master

- ✅ **Clear to compile and commit as part of the batch.** One file, `Private/SiegeLlamaSubsystem.cpp`,
  **plugin lane only** — no `Content/`, no header, no game module, no `.uproject`/`.uplugin`.
- ⚠️ **THE FILE IS UNTRACKED.** `git diff --stat` on it shows **nothing** and reads as *"changed
  nothing"* (§14 instance 3). **Do not use `--stat` as the integration proof; `git add` it.**
- ✅ **M8 declaration verified against the diff:** adds **no replicated property, no new replicated
  class, no new relevancy tier.** Two `FString::Printf` arguments inside a worker thread — **nothing
  reflected, nothing networked, no `UPROPERTY`/`UFUNCTION` surface touched.**
- ⛔ **If the build fails, the two changed arguments are the LEAST likely cause** (`double`→`double`,
  unchanged literals). Look at the batch's other files first.

## Carry-forward to TASK-447 — ⛔ THREE ITEMS THAT CHANGE WHAT 447 MAY CONCLUDE

1. ⛔ **On `tier=cpu`, absence of a `HARD TIMEOUT` line is NOT "no timeout"** — it is the
   `llama_decode=2` branch, which has no elapsed field (§5). **Use `code 2 + bCancelled false`.**
2. ⛔ **`Siege.Llama.SpikePrompt` / `SpikeBench` read the SPIKE's log, which still prints a
   CONFIGURED number** (`SiegeLlamaSpike.cpp:3144-3145`, §16-pinned). **447 must read the
   SUBSYSTEM's log specifically, or its "elapsed" figure is not a measurement.**
3. ⚠️ **Quote the corrected TASK-413 labels from WARN-1, not the comment's wording** — and quote
   the overshoot as **`printed − 10.0`**, remembering the ~50 ms rounding floor (§4).

## Board flips owed (orchestrator to apply — I do not edit the board)

- **TASK-461** → **`qa-passed`** (report `qa/TASK-461.md`, PASS, 0 blockers).
- **TASK-459** → **`qa-passed` / `ready-for-integration`**.
- **TASK-447** → ⛔ **UNBLOCKED** (its blocker was TASK-459's PASS).
