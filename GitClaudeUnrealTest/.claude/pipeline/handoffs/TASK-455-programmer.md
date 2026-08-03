# TASK-455 — gameplay-programmer handoff (2026-08-03)

**M8 DECLARATION DUTY, stated verbatim as required:**
> "adds no replicated property, no new replicated class, no new relevancy tier."

---

## ⛔ READ FIRST — FOUR THINGS THIS HANDOFF WILL NOT LET YOU BELIEVE

1. **NOTHING HERE HAS BEEN COMPILED OR RUN.** No compile, no editor, no MCP, no
   PIE (dispatch instruction; TASK-447 owns the one gate). **Every claim below is
   a claim about code, not an observation of behaviour.** §7 is the honest table.
2. **`ZoneBCharReserve` IS STILL 192 AND THAT IS THE SPEC'S OWN INSTRUCTION, NOT
   AN OMISSION.** The board's premise — *"`ReportFirstCapture` prints
   `zoneB_chars` from the shipped builder"* — is true about the **instrument** and
   false about the **reading**: that function **has never executed.** §4.
3. **TWO OF THE FOUR DELIVERABLES ARE REFUSED AS WRITTEN, ON A CHECKABLE
   MECHANISM (§15), AND THE REFUSAL IS THE POINT.** `MaxSnapshotTokens` and
   `SnapshotPreFilterMaxChars` **already exist**, in the plugin, where the
   enforcement is. Adding game-lane copies would have created two unenforced
   constants that *look* like guards. §3.
4. **THE `MaxSnapshotChars` COUNT IS 15 LINES / 16 OCCURRENCES — I COUNTED, AND
   MY COUNT AGREES WITH QA's 15 AGAINST TASK-450's "10".** Itemised by kind in
   §2, because the whole point of the correction is that a raw match count is not
   a call-site count.

---

## 1. Files changed — FOUR, all pre-existing, all edited ADDITIVELY

| file | tracked? | what I did |
|---|---|---|
| `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantSnapshot.h` | modified (dirty from TASK-441) | retired `MaxSnapshotChars`, introduced `SnapshotTrimBudgetChars`, rewrote `ZoneBCharReserve`'s comment |
| `...\Siegebound\SiegeAssistantSnapshot.cpp` | modified (dirty from TASK-441) | 8 lines re-pointed at the new name; two collapse-log strings corrected + widened |
| `...\Siegebound\SiegeAssistantComponent.h` | **untracked** (TASK-442/443) | `EnsureStaticPrefixRegistered` decl + 2 latches |
| `...\Siegebound\SiegeAssistantComponent.cpp` | **untracked** (TASK-442/443) | 3 `static_assert`s, the `SetStaticPrefix` caller + 2 call sites, audit-line update |

**⛔ REVERTED NOTHING.** Specifically preserved, checked by reading:

- **441's scoping decision is untouched.** `KindNotOrderable` still applies to
  Send/Guard/Ambush only; I did not go near `ValidateCommandAgainstSnapshot` or
  the Follow scoping note. *"clerics follow me"* still passes.
- **443's off-path structure is untouched.** Both of my call sites are in
  `NotifyConsoleOpened` and `DispatchTurnToModel` — the **dispatch** step, which
  runs before the model has answered. The confirm branch lives in
  `RouteParsedCommandInternal`, **after** the completion. ⛔ **Nothing I added
  sits between the guard and the confirm branch**, and `EnsureStaticPrefixRegistered`
  returns `void`, reads no FSM state and writes no FSM state — **no check
  anywhere became conditional on the toggle.**

**⛔ No plugin edits. No controller edits. No `Content/`. No Git. No compile.**

---

## 2. ⚖️ THE RETIREMENT — AND WHY RE-POINTING WOULD HAVE BEEN A DEFECT

### 2a. The reason, in the form that makes it checkable

`MaxSnapshotChars = 1085` was the **AUTHORITY** on the ≤400-token B+C budget. A
character cap standing in for a token budget is safe **only while it assumes the
SMALLEST plausible chars/token ratio** — a smaller ratio means the same characters
buy *more* tokens. The **PRE-FILTER** role that succeeded it **inverts** that duty:
a pre-filter must never reject what the authority would accept, so it must assume
the **LARGEST**.

⇒ **The same number is correct in one role and wrong in the other.** Carrying the
name across that boundary carries a safety label pointing the wrong way — and that
is not hypothetical here: **this exact lane already shipped that defect once**, the
`3.6` ratio labelled *"conservative"* while every measured reading (2.13 / 2.71 /
2.77 / 3.56) sat below it, over-admitting by ~33 %.

### 2b. ⚠️ BUT THE OBVIOUS EXECUTION OF "RETIRE IT" IS ALSO WRONG, AND THIS IS THE FINDING QA SHOULD RULE ON FIRST

**There is exactly one *code* reader of the old constant that does real work:**

```cpp
// SiegeAssistantSnapshot.cpp
const int32 RosterBudget = MaxSnapshotChars - ZoneBCharReserve - Head.Len() - Tail.Len();
```

**That is not an authority and it is not a pre-filter. It is a TRIMMER**, and it is
the only mechanism in the whole feature that **degrades gracefully** instead of
refusing. Both plugin bounds **REJECT** the turn (TASK-450 §5, deliberately —
*"the last line of the snapshot is the player's own sentence"*).

⇒ ⛔ **Had I pointed this at the pre-filter's 3000, the roster would have stopped
collapsing, a wide board would have sailed past the builder, and the tokenizer
would have REFUSED THE WHOLE TURN** — the player gets *"the assistant is
unavailable"* instead of an answer computed against a slightly narrower roster.
**That is a functional regression dressed as compliance.**

### 2c. ⇒ THE THIRD ROLE, DECLARED AS AN ADDITION (§15) RATHER THAN SLIPPED IN

**`static constexpr int32 SnapshotTrimBudgetChars = 1085;`** — new name, **value
deliberately unchanged**, direction **re-derived and written at the declaration**:

| role | constant | unit | lane | safe direction |
|---|---|---|---|---|
| **AUTHORITY** | `USiegeLlamaSubsystem::MaxSnapshotTokens` = **400** | tokens | plugin | *n/a — it IS the budget, counted by `llama_tokenize`* |
| **PRE-FILTER** | `USiegeLlamaSubsystem::SnapshotPreFilterMaxChars` = **3000** | chars | plugin | assume the **LARGEST** ratio |
| **TRIM BUDGET** | `USiegeAssistantSnapshot::SnapshotTrimBudgetChars` = **1085** | chars | **game** | assume the **SMALLEST** ratio |

⚖️ **THE VALUE DID NOT MOVE BECAUSE ONLY THE *PRE-FILTER* ROLE INVERTS, AND THIS IS
NOT THAT ROLE.** The trimmer wants the same direction the retired authority had. It
is `400 × 2.71` on two measured operands, exactly as before.

⛔ **AND I DID NOT RE-DERIVE IT, ON PURPOSE.** Moving it would move Zone C's bytes,
and **CONVENTIONS §12c freezes them for this wave** (*"ZONE B AND ZONE C BYTES DO
NOT MOVE IN THIS WAVE"* — the `t0` tripwire that bar #3's reproduction depends on).
⇒ **`BuildZoneC` is byte-identical on every board after this change.**

⚠️ **AND I WROTE THE HONEST LIMIT AT THE DECLARATION RATHER THAN LETTING THE WORD
"BUDGET" IMPLY MORE:** at Zone B's own stricter measured ratio (2.13) a 1085-char
snapshot can still be ~509 tokens, over the 400 the authority enforces. **This
budget makes that outcome unlikely; the tokenizer is what makes it impossible.**

### 2d. ⇒ THE TRUE COUNT — 15 LINES, 16 OCCURRENCES, ITEMISED

Captured by `grep -rn` over `Source/` **before any edit**, then classified by
reading each line. ⚠️ **`grep -rn` counts LINES; component `.cpp:2652` carried two
occurrences on one line**, which is why the two figures differ.

| kind | lines | where |
|---|---|---|
| **REAL CODE references** | **5** (6 occurrences) | `h:266` (the declaration) · `cpp:1398`, `:1445`, `:1455` · component `cpp:2652` (×2) |
| **prose inside `TEXT()` log strings** | **3** | `cpp:1443`, `:1453` · component `cpp:2646` |
| **prose inside `//` and `/* */` comments** | **7** | `cpp:1395`, `:1424`, `:1532` · `h:254`, `:269`, `:292`, `:672` |
| **TOTAL** | **15 lines / 16 occurrences** | 8 in `Snapshot.cpp` · 5 in `Snapshot.h` · 2 in `Component.cpp` |

- ✅ **My count agrees with QA's 15 exactly.**
- ⚠️ **TASK-450's figure was wrong and the error is the §14 instance-2 shape.** It
  reported *"`h:266` + `h:274` … plus **7 more uses** in the `.cpp` and **2** in
  `Component.cpp`"* ⇒ 11, summarised on the board as *"10 sites"*. Two separate
  faults: `Snapshot.cpp` has **8** matching lines, not 7; and **`h:274` is
  `ZoneBCharReserve`, a different symbol entirely**, counted into a
  `MaxSnapshotChars` total.
- ⚖️ **The number that actually matters is neither 15 nor 10: it is 5.** Only five
  lines were code. **Ten of fifteen matches were prose** — so a task that had
  "fixed 10 sites" and stopped could have left a live code reference behind, and a
  task that treated all 15 as code would have reported 10 phantom fixes.

**POST-EDIT STATE, verified on disk (§14 — not by a negative search):**

```
grep -rn "MaxSnapshotChars" Source/   ->  4 lines, 4 occurrences, ALL inside " * " doc-comment lines
                                          ZERO code references, ZERO log-string references
```

⚠️ **THOSE 4 SURVIVING MENTIONS ARE DELIBERATE AND I AM FLAGGING THEM RATHER THAN
LETTING QA FIND THEM.** §8's wording is *"a grep for `MaxSnapshotChars` returns
nothing."* **The symbol is gone; the four mentions are the retirement notice
itself** (three) plus one dated parenthetical in `MaxUtteranceBytes`' comment.
**This is the idiom this very file already established**, for
`llama_kv_cache_seq_rm`: *"the dead spelling is spelled out ONCE, here, on purpose
… so a reader who arrives from an older doc grepping the wrong name lands on this
correction instead of on nothing."* Given §14 (*a negative search proves nothing*),
landing on *"retired, here is why, here is the successor"* beats landing on
silence. ⚖️ **If QA rules the literal reading, deleting all four is a two-minute
edit and I will not argue — but it should be a ruling, not a drive-by.**

**OUT OF MY WRITE SCOPE, RECORDED SO IT IS NOT MISREAD AS A MISS:**
`SiegeLlamaSpike.cpp:174` declares **`SpikeMaxSnapshotChars`** — a *different
symbol*, an independent transcribed mirror, **print-only** (`:4633`), in the file
**§16 reclassified as load-bearing test infrastructure** and which my dispatch
forbids me to touch. `SiegeLlamaSubsystem.h:159` also names the retired game-lane
constant in a comment. **Both are now stale; neither is mine.** 📌 *Manager: one
line for whoever next opens the plugin.*

---

## 3. ⛔ DELIVERABLES 2 AND 3 — REFUSED AS WRITTEN, ON A MECHANISM (§15)

**The spec says: "Add `MaxSnapshotTokens`" and "Add `SnapshotPreFilterMaxChars`".
Both already exist. Verified by reading the file on disk (§14 — it is untracked,
so a `git grep` would have returned nothing and *proved nothing*):**

```
Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h:149   static constexpr int32 MaxSnapshotTokens = 400;
Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h:170   static constexpr int32 SnapshotPreFilterMaxChars = 3000;
```

Both are `public`, both are `static constexpr`, and **TASK-450's handoff states the
intent in terms**: *"The game lane reads these symbols directly, so there is
exactly one copy of every number."*

⇒ **Adding game-lane copies would have produced two constants that NOTHING IN THE
GAME LANE ENFORCES.** The game module has no tokenizer, so it cannot enforce
`MaxSnapshotTokens`; the pre-filter runs inside `RequestCompletion`, in the plugin.
⚠️ **A reader greps `MaxSnapshotTokens`, finds it in the game lane, and concludes
the game lane enforces it. That is a guardrail that reports safe — verbatim the
failure class this entire wave keeps finding, and the one TASK-450 refused to
commit by logging inertness at Warning.**

### ⇒ WHAT I SHIPPED INSTEAD: THE PROSE LAW, AS A COMPILE ERROR

Three `static_assert`s in `SiegeAssistantComponent.cpp` — **the only translation
unit that already sees both lanes' constants**, so this costs the snapshot header
**zero** new coupling (it is included by two automation-test TUs):

1. `SnapshotTrimBudgetChars < SnapshotPreFilterMaxChars` — ⚖️ **the mechanical form
   of §8's law**: *"A PRE-FILTER THAT CAN REJECT WHAT THE AUTHORITY WOULD ACCEPT IS
   NOT A PRE-FILTER. IT IS A SECOND, HIDDEN AUTHORITY."* Today 1085 vs 3000. **If
   anyone ever crosses them, the build stops.**
2. `ZoneBCharReserve < SnapshotTrimBudgetChars` — a reserve at or above the budget
   drives `RosterBudget` negative and prints an **empty roster on every board**,
   reachable by a one-line constant edit.
3. `2 × MaxUtteranceBytes < SnapshotTrimBudgetChars − ZoneBCharReserve` — spec item
   (5) / `qa/TASK-416.md` WARN-1, mechanised. ⚠️ **Labelled NECESSARY-NOT-SUFFICIENT
   in the comment**: head and tail also spend budget, so clearing it does not prove
   the roster gets a usable slice.

⚖️ **A prose law in a 500 KB document is checked by whoever remembers to read it; a
`static_assert` is checked by the compiler on every build.** ⚠️ **These are the
first things TASK-447 will hit if they are wrong** — a failure here is a *correct*
compile failure, not a defect in my code.

⛔ **IF QA DISAGREES, THE FIX IS CHEAP AND I WILL TAKE IT** — but the fix should be
*aliases* (`= USiegeLlamaSubsystem::MaxSnapshotTokens`), never fresh literals, and
it costs the snapshot header a plugin include.

---

## 4. ⛔ `ZoneBCharReserve` STAYS 192 — THE SPEC'S OWN FALLBACK CLAUSE, FIRED

The spec: *"SET `ZoneBCharReserve` FROM THE SHIPPED BUILDER'S **PRINTED** WORST
CASE … ⚠️ If you cannot obtain a printed figure (no editor), **STATE THAT AND LEAVE
THE CONSTANT ALONE** — a derived number is not a measurement."*

**Both candidate instruments fail on the word *printed*:**

| instrument | why it does not yield a printed figure |
|---|---|
| `Siege.Llama.SpikePrompt` (68 t0 / 71 t1) | ⛔ measures the **spike's `AppendZoneB`**, not this file's `BuildZoneB`. A measurement of the other lane is not a measurement of this one — **§12g carries a standing WARN making exactly that point about Zone A** (*"`zoneA_chars=5116` PROVES THE *SPIKE* LANE ONLY"*). |
| `USiegeAssistantComponent::ReportFirstCapture` | ✅ prints `zoneB_chars` from the **shipped** builder — ⛔ **and it has NEVER EXECUTED.** The batch is uncompiled, TASK-447 is the single gate, and my dispatch forbids editor/MCP/PIE. |

⚖️ **THE DISPATCH'S PREMISE IS TRUE ABOUT THE INSTRUMENT AND FALSE ABOUT THE
READING, AND THE DIFFERENCE IS THE WHOLE TASK.** *"You no longer need the spike"*
is correct — `ReportFirstCapture` **is** the better instrument, and it is the one
§8 was waiting for. But **an instrument is not a measurement**, and *"observable is
not observed"* is a phrase §8 already had to add once, about this same class.

⛔ **I could have derived ~85 from the four format strings and it would probably be
right.** I did not, because **that is the precise defect §12g exists to prevent**,
and this constant's own history is the argument: **the 192 it would replace was
itself eyeballed.** Replacing one un-measured number with another un-measured
number and calling it a re-measurement is how the first one got here.

### ⇒ I MADE THE READING TAKEABLE INSTEAD OF GUESSING IT

`ReportFirstCapture` now prints the reserve, the live `zoneB_chars`, **and the
over-charge**, with the instruction attached:

> `>>> OWED READING — THIS IS THE LINE TASK-455 NEEDED AND COULD NOT TAKE.
> ZoneBCharReserve is 192 and this board's zoneB_chars is N … size it from this
> figure PLUS the widest value each of the four fixed keys can take — both castles
> at 100%, mid: neutral, a four-digit gold band — because one live board is a
> SAMPLE, not a WORST CASE. Lowering it can only WIDEN the roster.`

⚠️ **That last clause matters and is why this is safe to leave open:** the reserve
only ever moves **down**, and moving it down only ever **widens** the roster. **The
current 192 is the conservative end.** Nothing is unsafe today; the prompt is
merely ~124 chars narrower than it needs to be.

📌 **MANAGER: this is a one-line follow-up task, and it needs the editor + a
compiled build — nothing else.**

---

## 5. ⛔ THE MISSING CALLER — AND EXACTLY WHICH GUARDS ARE NOW LIVE

`USiegeAssistantComponent::EnsureStaticPrefixRegistered(USiegeLlamaSubsystem*)`,
called from **two** places.

### 5a. Why it was not one call

| call site | why |
|---|---|
| `NotifyConsoleOpened` | ⚠️ **EARLY, AND "EARLY" IS THE ENTIRE REASON.** The plugin's **char pre-filter reads the *APPLIED* prefix on the game thread inside `RequestCompletion`** — so a prefix registered in the same frame as the request **is too late for it**; the worker has not tokenized it yet. Registering at console-open buys the worker the seconds the player spends typing. **This is what makes the pre-filter live on the FIRST sentence.** |
| `DispatchTurnToModel` | The **backstop**, for the session where the model became ready *between* the open and the sentence. Placed **after** the `IsBusy()` guard (`SetStaticPrefix` refuses while a request is in flight) and **before** `RequestCompletion`. |

### 5b. ⛔ THE `IsReady()` GATE IS THE CORRECTNESS OF THE FUNCTION, NOT AN OPTIMISATION

`SiegeLlamaSubsystem.cpp:1039-1047` — `ApplyPendingStaticPrefix` **DISCARDS** a
prefix that arrives before the model has loaded (nothing to tokenize with) and says
so **on the worker**. ⚠️ **`SetStaticPrefix` would still have returned `true`.**

⇒ An eager call would have **latched, logged a cheerful success, and left both
budgets inert for the entire session.** ⛔ **That is the exact "a guard that cannot
see says nothing and reads as a pass" shape this task exists to close** — recreating
it one layer up would have been the worst possible way to close it. Not-ready is
**silent and unlatched**: it is the normal state for the first seconds of a match.

### 5c. GUARD STATUS AFTER THIS TASK — the table the coordinator asked for by name

| guard | before | **after** |
|---|---|---|
| CONTEXT budget (`PromptTokens + MaxOutputTokens ≤ n_ctx_actual`) | ✅ always enforced | ✅ **unchanged — always enforced, no unguarded path to `llama_decode` in any interleaving** |
| `MaxSnapshotTokens = 400` **authority** | ⛔ INERT | ✅ **LIVE** from turn 1 (see the race note) |
| `SnapshotPreFilterMaxChars = 3000` **pre-filter** | ⛔ INERT | ✅ **LIVE** from turn 1 via the console-open registration; **turn 2** if that path was skipped |
| §8 ZONE-A SIZE BOUND assertion | ⚠️ ran with `ZoneA_tokens` = 0 | ✅ **COMPLETE** — re-runs on registration with a **MEASURED, tokenized** `ZoneA_tokens` |

⇒ **TASK-450's three inertness Warnings should stop firing.** ⛔ **That is a
prediction about unexecuted code, not an observation — TASK-447 confirms it by
their absence plus the presence of `LLM_BUDGET … STATIC PREFIX REGISTERED: N chars
-> N tokens`.**

✅ **BONUS THIS UNLOCKS, WORTH NAMING:** that line is **the shipped lane's first
ever measured `zoneA_tok`** — §12g's standing WARN (*"NO COMMAND PRINTS THE SHIPPED
`BuildZoneA`"*) is discharged by running the console once, **without the spike**.

### 5d. ⚠️ TWO HONEST GAPS I AM DECLARING RATHER THAN ROUNDING OFF

1. ⚠️ **THE TURN-1 AUTHORITY CLAIM IS A RACE, NOT A GUARANTEE.** The worker's loop
   runs `ApplyPendingStaticPrefix()` **then** `if (bWorkPending)` — so a same-frame
   registration is normally applied first. **But the worker can be sitting between
   those two statements**, in which case the prefix applies on its next iteration
   and that one request runs unmeasured. The window is a few instructions wide and
   is reached only on a wake. ⚖️ **I wrote it into the code comment as a race
   rather than asserting an ordering I cannot lock** — CONVENTIONS §15's third
   instance is exactly an unverified ordering asserted as fact. ✅ **Worst case is
   benign: the budget goes live one turn later, and the CONTEXT budget is enforced
   either way.**
2. ⚠️ **`bStaticPrefixRegistered` RECORDS A *SUBMISSION*, NOT AN *OUTCOME*.** The
   worker can still fail to tokenize, on its own thread; the pinned §9 surface
   gives this lane no way to read that back. **The plugin's own Warning is the only
   signal**, and the success log says so in terms (*"a success here is a
   submission, not an application"*). ⛔ I did **not** add a cross-lane accessor to
   fix this — the remedy for an unobservable failure is the log that already
   exists, not a widening of the lane surface.

---

## 6. SPEC ITEMS (4) AND (5)

**(4) OBSERVABLE TRUNCATION, RE-ANCHORED ONTO *THE ACT*.**
⇒ ✅ **I ADDED ZERO NEW CUT SITES.** Stated plainly because the clause is about new
ones. What I added is the **activation** of two existing sites that both **REJECT
and both log at Warning** (TASK-450 §5). **No mechanism anywhere in this change
shortens the snapshot silently.** The builder's own collapse log is untouched and
still escalating; I widened its Warning text to name the real authority so a reader
of the log cannot mistake the trim budget for the cap.

⚠️ **AND ACTIVATION CHANGES PLAYER-VISIBLE BEHAVIOUR — SAY SO:** a turn that would
previously have been sent can now be **refused** by `RequestCompletion` returning
false. ✅ **That path was already correct and I verified it rather than assuming
it:** `SubmitUtterance` handles `false` by clearing `bModelDispatchedThisTurn`,
returning to `Composing`/`Idle` and pushing `RefusedAssistantUnavailable`. **No
hang, no stuck `Thinking` state.**

**(5) `MaxUtteranceBytes` BOUNDS BOTH LINES, NEITHER TRUNCATED BY THE BUDGET.**
⇒ ✅ **Already structurally true, now asserted and documented.** Both sanitized
lines sit in `BuildZoneC`'s `Tail`, and `RosterBudget = SnapshotTrimBudgetChars −
ZoneBCharReserve − Head − Tail` subtracts **both** before the roster is given
anything — the roster absorbs all of it, which is §8's rule that the utterance is
never truncated by the snapshot budget. Worst case, both lines at their cap:
`1085 − 192 − ~137 head − ~566 tail ≈ 190 chars` for the roster ⇒ **a deep collapse
that is LOGGED at Warning with its cause named.** That is WARN-1 confirmed,
bounded, and observable rather than denied. Mechanised as `static_assert` (3).

📌 **BOARD NAME CORRECTION (§14 — checked at the artifact):** the `names:` list
says **`MaxUtteranceChars`**. The shipped symbol has been **`MaxUtteranceBytes`**
since TASK-433 renamed it (`SiegeAssistantSnapshot.h`), *"and the rename is the
fix, not cosmetics"*. **A task working from the board's spelling would grep for a
symbol that does not exist and conclude the cap is missing.**

---

## 7. ⚠️ WHAT CANNOT BE VERIFIED WITHOUT A COMPILE / A RUN

| claim | status |
|---|---|
| the three `static_assert`s hold | ⛔ **UNVERIFIED — first thing TASK-447 hits.** All three hold on today's values by hand (1085<3000 · 192<1085 · 480<893). |
| `SnapshotTrimBudgetChars` resolves at all 5 former code sites | ⛔ **UNVERIFIED.** Public `static constexpr` on the same class; the 4 in-class uses are unqualified, the 2 in the component are `USiegeAssistantSnapshot::`-qualified. |
| `BuildZoneC` is byte-identical to before | ⛔ **UNVERIFIED BY EXECUTION** — argued from the value being unchanged (1085) and no expression restructured. **§12c's `t0` tripwire is the test.** |
| the guards actually go live | ⛔ **UNVERIFIED.** Confirmed only by TASK-450's Warnings **ceasing** + the plugin's `STATIC PREFIX REGISTERED` line appearing. |
| `zoneB_chars` from the shipped builder | ⛔ **NOT MEASURED — that is §4, and it is the point.** |
| §13 (`TestEqualSensitive`) | ✅ **DOES NOT BIND — I ADDED NO TEST AND NO `TestEqual`.** Recorded so its absence is not read as an oversight. |

---

## 8. WHAT QA SHOULD SCRUTINISE — RANKED

1. ⛔ **RULE ON `SnapshotTrimBudgetChars` EXISTING AT ALL (§2b/§2c).** It is a
   third constant the `names:` list does not mention. My argument is that the one
   real code reader is a **trimmer**, not a bound, and that pointing it at 3000
   converts graceful degradation into a refused turn. **If you disagree, say what
   the roster trim should be sized against instead** — deleting it is not an option
   that leaves working code.
2. ⛔ **RULE ON THE REFUSAL OF DELIVERABLES 2 AND 3 (§3).** I refused to create
   game-lane copies of two constants that already exist in the plugin. This is the
   biggest departure in the task.
3. ⛔ **RULE ON THE 4 SURVIVING COMMENT MENTIONS (§2d)** against §8's literal *"a
   grep returns nothing"*. I invoked this file's own dead-spelling idiom; it is an
   interpretation.
4. **CHECK MY COUNT (§2d).** 15 lines / 16 occurrences / **5 real code lines**. If
   your denominator differs, mine is wrong — the classification was done by reading
   each line, and ⚠️ **`Grep` mangles comment syntax on this machine**, so do any
   comment-level finding on raw `Read` output.
5. **THE TWO CALL SITES vs 443's OFF-PATH STRUCTURE.** Convince yourself
   `EnsureStaticPrefixRegistered` cannot make any machine check conditional on the
   confirm toggle. I claim it cannot: it returns `void`, touches no FSM state, and
   both sites are on the **dispatch** side of the model call.
6. **THE `IsReady()` GATE (§5b).** If it is wrong, both budgets are inert for the
   session **and the log says they are live.** This is the highest-consequence line
   in the change.
7. **THE RACE I DECLARED (§5d.1).** Rule on whether a one-turn lag in the worst
   case is acceptable, or whether the registration must move somewhere with a
   happens-before guarantee.
8. **THE COUPLING TO TASK-450's `SetStaticPrefix` RULING.** ⚠️ TASK-450 flagged that
   method as *"the one that needs a ruling"* — a declared widening of the §9 lane
   surface, and the coordinator reports §8's clauses were ruled genuinely
   self-contradictory. ⛔ **If QA rules `SetStaticPrefix` out, my caller dies with
   it and §8's mandatory assertion has no implementable form.** I did not hit an
   unsatisfiable instruction myself — I am the *caller*, and 450 had already
   resolved the contradiction — but **the two stand or fall together.**
