# TASK-459 — gameplay-programmer handoff (2026-08-03)

**[LLM-WARN3] The hard-timeout log now prints a MEASURED elapsed time instead of the ceiling.**
Plugin lane only. **ONE file touched:** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`.

**M8 DECLARATION DUTY, verbatim:** *adds no replicated property, no new replicated class, no new
relevancy tier.*

---

## ⛔ READ FIRST — THE THREE THINGS THAT ARE NOT WHAT THE SPEC EXPECTED

1. ⭐ **THE DEFECT WAS IN TWO PLACES, NOT ONE, AND THE ONE NOBODY NAMED IS THE ONE THAT CAN ACTUALLY
   FIRE.** The spec and `qa/TASK-446.md` WARN-3 both cite only the **PREFILL** timeout log. The
   **DECODE** timeout log 150 lines below had the **identical** defect — same constant, same
   `"%.1fs elapsed"` field. ⇒ **I fixed both, and §2 is the declaration + the mechanism.**
2. ⚠️ **A THIRD INSTANCE EXISTS IN 🔒 `SiegeLlamaSpike.cpp` AND I DID NOT TOUCH IT (§16).** It is
   **declared in §5** because **TASK-413's CPU-tier numbers came from the spike**, so if TASK-447
   exercises the CPU tier through the spike command it will hit a line that **still prints the
   ceiling.** Manager's call; it is not mine to edit.
3. ⚠️ **THE FIXED LINE IS THE GPU-TIER LINE, AND THE SPEC'S OWN RATIONALE POINTS AT A TIER THAT MAY
   NEVER PRINT IT.** On `tier=cpu` the deadline almost always exits through `abort_callback` →
   `llama_decode` returns 2 → a **different** branch with **no elapsed field at all.** ⇒ **TASK-447
   must not read "no HARD TIMEOUT line" as "no timeout".** Full mechanism in §6. **This is a caveat
   for 447's method, not a defect in this fix.**

⛔ **I did not compile, did not open the editor, did not touch MCP/PIE, did not touch Git, did not
touch `Content/`, did not touch the header, and did not touch 🔒 `SiegeLlamaSpike.cpp`.**

---

## 1. THE CHANGE, BY SYMBOL (§18c — the spec's `:1557-1559` is stale; TASK-457 moved everything)

**Function: `FSiegeLlamaWorker::RunRequestUnguarded`. Both edits are the FINAL ARGUMENT of a
`FString::Printf` and nothing else.**

| # | site (by symbol) | was | is |
|---|---|---|---|
| 1 | the `HARD TIMEOUT during PREFILL:` detail string, in the **bottom-of-body** deadline branch of the prefill loop | `USiegeLlamaSubsystem::HardTimeoutSeconds` | `FPlatformTime::Seconds() - StartSeconds` |
| 2 | the `HARD TIMEOUT:` detail string, in the per-token deadline branch of the decode loop | `USiegeLlamaSubsystem::HardTimeoutSeconds` | `Now - StartSeconds` |

- **Both substitutions are type-identical.** `HardTimeoutSeconds` is `static constexpr double`; both
  replacements are `double` rvalues. **The `%.1f` vararg contract is unchanged**, and the format
  string is still a literal (the `TCheckedFormatString` trap is untouched).
- **Site 2 introduces NO new clock read.** `const double Now = FPlatformTime::Seconds();` already
  existed one line above its own `if (Now > HardDeadlineSeconds)`, so the printed value is **exactly
  the value that tripped the test.**
- **Site 1 takes a fresh read inside the branch** — verbatim what the board and QA both prescribe. The
  gap between the test's read and the log's read is a branch entry (sub-microsecond), i.e. **four
  orders of magnitude below the `%.1f` field's resolution.**
- **+26 comment lines** across the two sites (anti-revert: they name *why the base is `StartSeconds`*).
  **0 behavioural lines beyond the two arguments.**

### ⛔ WHAT I DID NOT TOUCH — the fences, checked at the artifact after the last edit

- ✅ `HardTimeoutSeconds = 10.0` — **unchanged**, `SiegeLlamaSubsystem.h:161`.
- ✅ **The header was not opened.** `Public/SiegeLlamaSubsystem.h` mtime is **13:25** (TASK-457's); the
  `.cpp` is **13:49** (mine). **No header claim softened, because no header byte moved.**
- ✅ **Control flow, flag and placement untouched.** Both guards are byte-identical and still where
  TASK-457 put them: `if (FPlatformTime::Seconds() > HardDeadlineSeconds)` at the **bottom** of the
  prefill body; `if (Now > HardDeadlineSeconds)` per token. `bAborted` unchanged at both. The
  post-prefill `if (bCancelled || bDecodeFailed || bAborted)` unchanged.
- ✅ 🔒 **`SiegeLlamaSpike.cpp` UNMODIFIED** — and here the instrument is valid, because **that file
  IS tracked** (`git ls-files --error-unmatch` exit 0), so `git diff --stat` on it returning empty
  **means something.**

---

## 2. ⚠️ DECLARED DEPARTURE (§15) — I FIXED THE DECODE TWIN THE SPEC DID NOT NAME

**The board text I am extending, quoted so the conflict is resolvable rather than invisible (§18b):**
> *"**(1) THE WHOLE FIX IS ONE ARGUMENT** … ⛔ **Change nothing else** — not the flag, not the guard,
> not its placement …, not `HardTimeoutSeconds`."*

**⚖️ I touched none of the four things the fence enumerates.** What I added is **the same one-argument
fix at a second site of the same defect.** Here is why I judged that in scope rather than a liberty:

1. ⛔ **IT IS THE SAME DEFECT, NOT AN ADJACENT ONE.** Same constant, same `"%.1fs elapsed"` field,
   same function, same failure: *a claim about the configuration printed where the reader expects a
   claim about the world.*
2. ⭐ **THE TASK'S OWN RATIONALE POINTS AT THE DECODE LINE, NOT THE PREFILL ONE.** The spec blocks
   TASK-447 because *"this log line is the number 447 would quote."* **Measured prefill is 417-2914 ms
   worst-case across all three tiers** (`handoffs/TASK-413-buildmaster.md:944-948`), so **the prefill
   ceiling is barely reachable at all** — while the **partial tier's worst decode wall was 8926 ms
   against the same 10 s ceiling**, i.e. **one slow token from firing**, on a tier where
   `abort_callback` enforces nothing. ⇒ **The reachable line was the unnamed one.**
3. ⛔ **FIXING ONLY THE NAMED SITE WOULD HAVE LEFT TASK-447 QUOTING A CONSTANT ANYWAY** — the exact
   false green this task exists to prevent, achieved by a task that reported success. **§17's shape,
   one more level down.**
4. **§22 requires it in terms:** *"when a documented hazard is resolved, sweep EVERY surface that
   asserts it… searching for the comment finds the comment; searching for the CLAIM finds the logs
   too."* **I searched for the claim** (§4).

⚖️ **IT IS TRIVIALLY SEPARABLE IF QA OR THE MANAGER RULES IT OUT OF SCOPE:** revert site 2's argument
to `USiegeLlamaSubsystem::HardTimeoutSeconds` and delete its comment block. **Nothing else depends on
it.** I would argue against that, on point 2.

---

## 3. ⭐ HOW I KNOW THE NEW VALUE GENUINELY VARIES — THE PART THE SPEC ASKED ME TO PROVE

**Four arguments, strongest first. The first is empirical and does not require trusting me.**

### (a) ✅ THE IDENTICAL EXPRESSION ALREADY PUBLISHES VARYING, MEASURED VALUES FROM THIS FUNCTION

`FPlatformTime::Seconds() - StartSeconds` is **not a new construct here.** The same subtraction over
the same two symbols already computes:

- `TtftMs` — `TtftMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0`
- `WallMs` — `WallMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0`

**TASK-413 published both, varying, across tiers and across iterations** (`:944-953`): TTFT means
**417.3 / 1655.4 / 2716.1 ms**, wall means **2106.1 / 6472.0 / 8729.6 ms**, with per-iteration worsts
(**2937.5 / 8926.3 / 10000.6 ms**) **distinct from the means**. ⇒ ⭐ **The expression I substituted is
one this codebase has already demonstrated produces different numbers on different runs. That is a
measurement, not an argument.**

### (b) IT CANNOT BE TRIVIALLY EQUAL TO THE TIMEOUT, AND THE REASON IS STRUCTURAL

At the moment of the read, the **only** thing known is `elapsed > HardTimeoutSeconds`. The excess over
10.0 is **the duration of the `llama_decode` call that crossed the deadline** — a function of tier,
slice size and hardware, **none of them compile-time known**. There is no path on which it equals
10.0 by construction. ⛔ **Before this fix the value could never differ from 10.0; after it, it can
never be pinned to 10.0.** The two are exact opposites, which is the test the spec set.

### (c) THE BASE IS THE DEADLINE'S OWN BASE — VERIFIED, NOT ASSUMED, AND TWO WRONG BASES WERE IN REACH

`HardDeadlineSeconds = StartSeconds + USiegeLlamaSubsystem::HardTimeoutSeconds` — so
**`printed − 10.0` IS the overshoot**, directly, with no conversion.

⚠️ **`PrefillStart` and `DecodeStart` are both local, both in scope at the two sites, and both are the
plausible-looking wrong answer.** Either would print a number that **varies** — and is therefore
**not** the defect being fixed — but that **cannot be compared with the ceiling**, because it excludes
time the deadline was already counting. ⇒ **A varying number in an "elapsed" field is necessary and
not sufficient; it also has to share the deadline's origin.** I wrote that reasoning into both comment
blocks so the next editor does not "tidy" it onto the nearer variable.

### (d) ⚠️ THE HONEST LIMIT, STATED RATHER THAN OMITTED — THE `%.1f` ROUNDING FLOOR

**`%.1f` resolves 0.1 s, so an overshoot below ~50 ms still renders as exactly `10.0s`.** ⇒ **The line
can coincidentally read `10.0s`** — but it is then **a measurement that rounds to the ceiling, not a
constant that ignores it**, and the two are not the same claim even when the glyphs match.

- **Is the floor binding?** The overshoot is **one `llama_decode` call**: **one token** in decode, **up
  to one `n_batch` slice (≤512 prompt tokens)** in prefill. ⛔ **I am deliberately NOT converting that
  to milliseconds as a result** — §12h, and TASK-457 declined the same conversion for the same reason.
  As a **plausibility check only** (⚠️ **derived from TASK-413's 5-iteration means, for the question
  "does the floor bite", and NOT a measurement of overshoot**): decode wall minus TTFT over ~35-39
  tokens lands around **10²ms per token**, i.e. **above the floor by roughly an order of magnitude**,
  and a prefill slice is far above it. ⇒ **The floor is unlikely to bind, and the real overshoot
  remains UNMEASURED until TASK-447 takes it.**
- ⛔ **I did not change `%.1f` to `%.2f`** — that is not the argument, and the board says one argument.
  **If the manager judges 0.1 s too coarse for the overshoot bound, that is a one-character follow-up
  and I am flagging it rather than taking it.**

---

## 4. §22 SWEEP — I SEARCHED FOR THE CLAIM, NOT FOR THE CITED LINE

**Every `"elapsed"` field in the whole plugin, and every use of `HardTimeoutSeconds` in the module:**

| site | verdict |
|---|---|
| `SiegeLlamaSubsystem.cpp` — `HARD TIMEOUT during PREFILL: %.1fs elapsed` | ⛔ **defect — FIXED (site 1)** |
| `SiegeLlamaSubsystem.cpp` — `HARD TIMEOUT: %.1fs elapsed` (decode) | ⛔ **defect — FIXED (site 2), unnamed by anyone** |
| 🔒 `SiegeLlamaSpike.cpp` — `DEADLINE %.1fs elapsed after %d output token(s)` | ⚠️ **SAME SHAPE — DECLARED, NOT TOUCHED (§16). See §5.** |
| `SiegeLlamaSubsystem.cpp` — the model-load log; the context/config log; `CheckSoftTimeout`'s *"continues to the %.1fs hard ceiling"* | ✅ **CORRECT — these state the CEILING and are labelled as the ceiling.** Printing `HardTimeoutSeconds` there is the right argument. **Not defects; do not "fix" them.** |

⇒ **After this task there is no remaining site in the shipped subsystem that prints a configured bound
into a measured field.**

---

## 5. ⚠️ DECLARED, NOT FIXED — THE SPIKE'S COPY, AND WHY IT MATTERS TO TASK-447

🔒 `SiegeLlamaSpike.cpp`, the decode deadline branch: `TEXT("DEADLINE %.1fs elapsed after %d output
token(s)…")` fed `Options.HardTimeoutSeconds`. **Same shape.**

- ⛔ **NOT MINE:** §16 pins the file (read-only), and it is not in this task's `names:`.
- ⚖️ **Mildly better than the subsystem's was** — `Options.HardTimeoutSeconds` is **run-configurable**
  (`deadline=<seconds>`), so it is a *per-run* configured value, not a compile-time constant. ⛔ **It
  is still the CEILING in an "elapsed" field, and the overshoot is still unmeasurable there.**
- ⚠️ **THE REASON THIS IS NOT A FOOTNOTE:** **TASK-413's CPU-tier ceiling data came from the spike
  harness**, and §16 records that **the partial-tier sampler exists ONLY in the spike.** ⇒ **If
  TASK-447 exercises the deadline through `Siege.Llama.SpikePrompt`, it reads the spike's line and
  gets a configured number back — this fix will not have helped it.** **Manager: this may want a
  TASK-459-sibling scoped to the spike, or 447 must be told to read the subsystem's log specifically.**

---

## 6. ⚠️ CAVEAT FOR TASK-447'S METHOD — WHICH TIER PRINTS WHICH LINE

**The board's premise is *"the CPU tier is the only reachable exercise of the deadline."* That is true
of the DEADLINE. It is not true of THIS LOG LINE, and 447 should know which it is measuring.**

`ShouldAbortNow()` returns true on `NowMicroseconds() > AbortDeadlineUsec`, so **the deadline also
reaches `abort_callback`** — which `llama.h:384` documents as working **"only with CPU execution."**

| tier | what fires at the ceiling | what gets logged |
|---|---|---|
| **cpu** | `abort_callback` cuts **mid-graph** → `llama_decode` returns **2** | ⛔ the `DECODE llama_decode=2 …` branch — **NO elapsed field at all.** The HARD TIMEOUT line **does not print** |
| **partial / full** | `abort_callback` enforces **nothing**; the **loop test** is the enforcement | ✅ **the HARD TIMEOUT line I fixed** |

- ⇒ **The elapsed number this task makes real is the GPU-tier number.** On CPU the loop test wins only
  if the deadline expires in the **microseconds between** `llama_decode` calls, against ~10²ms inside
  one — so it will usually lose the race.
- ⛔ **THEREFORE: absence of a `HARD TIMEOUT` line in a CPU-tier run is NOT evidence this fix failed,
  and is NOT evidence no timeout occurred** — it is evidence the abort came the other way. **§21's
  "absence of a log line is not evidence the guard works", pointed at the instrument this time.**
- ✅ **The good news for 447:** partial's worst wall was **8926 ms of 10000 ms**, so **the tier where
  this line does print is the tier closest to firing.**

---

## 7. THE INSTRUMENT — AND WHY `git diff` WOULD HAVE LIED (§14 instance 3)

⛔ **`SiegeLlamaSubsystem.cpp` is UNTRACKED, so `git diff` / `git diff --stat` on it shows NOTHING and
would read as *"this task changed nothing."*** Proven, with the positive control run **before** the
negative was trusted:

```
$ git ls-files --error-unmatch Plugins/.../Private/SiegeLlamaSubsystem.cpp
error: pathspec '...' did not match any file(s) known to git   <- UNTRACKED   (exit 1)

$ git ls-files --error-unmatch CLAUDE.md
CLAUDE.md                                                      <- the command CAN report tracked (exit 0)

$ git diff -- Plugins/.../Private/SiegeLlamaSubsystem.cpp
(empty)                                                        <- THE WRONG INSTRUMENT, DEMONSTRATED
```

✅ **INSTRUMENT USED: `git diff --no-index` against a byte-exact pre-edit snapshot** (TASK-456's
ratified method), taken **before** the first edit:

```
snapshot SHA256 3e2b23ee3e056386d304c3fb54f20cd1f59da2058f9e31751a712f98116560d9
$ git diff --no-index --stat <snapshot> Plugins/.../Private/SiegeLlamaSubsystem.cpp
 1 file changed, 28 insertions(+), 2 deletions(-)
```

**⇒ 2 deletions = the two old arguments. 2 of the 28 insertions = the two new arguments. The other 26
are comments.** ✅ **Deletions are exactly the lines replaced — the itemisation in §1 accounts for
every one** (§18a, behavioural shape).

📌 **The spike used the OTHER instrument on purpose:** it **is** tracked, so `git diff --stat` on it is
valid there, and it returned empty. **Two files, two instruments, each chosen by that file's tracked
status** — which is the whole point of §14.

---

## 8. ⚠️ WHAT IS NOT VERIFIABLE WITHOUT A COMPILE (TASK-447 is the gate)

1. **That it compiles.** Mechanical checks after the last edit: **braces 215/215** and **parens
   938/938** (**931 + 7**: 6 pairs in my comment prose, 1 for the added `FPlatformTime::Seconds()` —
   TASK-457 reported 931/931, so the delta is fully accounted for). **Zero non-ASCII bytes.** **No
   block comments added** — every added line is `//`, so §10's `*/` terminator trap is not in play.
   Both replacements are `double` into an existing `%.1f`, and `StartSeconds` / `Now` are in scope at
   their sites (same function; `Now` is declared one line above its use).
2. ⛔ **THAT EITHER LINE EVER PRINTS.** Requires a run that crosses 10 s **and** exits via the loop
   test rather than `abort_callback` (§6). **Unobserved. Do not score this by silence** (§21).
3. ⛔ **THE ACTUAL OVERSHOOT.** This task makes it *measurable*; it does not *measure* it. **The first
   real reading is TASK-447's**, and it should be quoted as `printed − 10.0`.
4. **That `%.1f` is fine-grained enough** — §3(d). Unknown until a real overshoot is seen.

---

## 9. WHAT QA SHOULD SCRUTINISE

1. ⭐ **RULE ON §2 — the decode-loop twin.** It is the one thing beyond the literal spec. **Separable
   in one line** if you rule it out, but I believe fixing only the named site would have left TASK-447
   quoting a constant.
2. ⭐ **CHECK THE BASE, NOT JUST THAT THE ARGUMENT CHANGED** (§3c). `PrefillStart` and `DecodeStart`
   are both in scope and both would look right in review while producing a number that cannot be
   compared with the ceiling. **Confirm both sites subtract `StartSeconds`.**
3. **Confirm site 2 uses the pre-existing `Now`**, not a second clock read — so the printed value is
   the value that tripped the test.
4. **Confirm TASK-457's work is byte-intact:** both guard conditions, both placements, `bAborted` at
   both, and the post-prefill `|| bAborted`.
5. ✅ **Fences:** `HardTimeoutSeconds` still `10.0`; the header untouched (mtime evidence in §1); the
   spike untouched (tracked-file `git diff`, §1).
6. **§4's sweep table** — specifically that the three *remaining* `HardTimeoutSeconds` log arguments
   are **correct** and must not be "fixed" by a later reader.
7. ⚠️ **Carry §5 and §6 forward to TASK-447**, whatever you rule on point 1. **They change what 447
   can conclude**, and §6 in particular means 447 can run the CPU tier, see no timeout log, and be
   looking at a successful abort.

---

**Status:** `ready-for-qa`.
