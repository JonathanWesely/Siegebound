# QA Report — TASK-464 (RETROACTIVE scoped gate on TASK-442)

**Verdict: PASS on TASK-442 — 0 BLOCKERS, 6 WARN, 3 NIT.**

- Subject: **TASK-442 only** — `USiegeAssistantComponent`: the FSM skeleton, the reason-code / template table, the clarification short-circuit, the fault latch, the authority refusal, the M8 declaration.
- Files reviewed at their **current** committed state: `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` (1417 lines) · `SiegeAssistantComponent.cpp` (2820 lines) · `handoffs/TASK-442-programmer.md`.
- ⛔ **The commit `cd5f4ed` STANDS. No finding in this report routes to a revert** — every one routes to a fix task, and the disposition is the manager's. Reverting green, in-file-reviewed code over what is below would be strictly worse than the breach this gate remediates.
- ⚠️ **§18c honoured: every citation below was verified BY SYMBOL at the current file.** Every offset in TASK-442's handoff and spec is stale (443 · 455 · 456 have edited this pair since 442 authored it); no finding here rests on a moved number.

---

## ⛔ STANDING LIMITS — STATED VERBATIM, AS REQUIRED

- **No automation test in this repo has ever run. 33 compile; compiling is not passing.**
- **WARN-5 remains instrumented, not discharged.**
- **Bar #5 stands uncleared at 20/25 vs 22 — nothing here is accuracy progress.**
- **`DA_AssistantVocabulary` does not resolve (found at first execution), so the shipped synonym table is empty. TASK-463 owns that.**
- **The code is ALREADY COMMITTED (`cd5f4ed`; both DLLs linked, zero errors, zero warnings) and NO SMOKE ROW HAS RUN, so every behavioural claim in this report is about SOURCE, not about observed runtime.**

Where `DA_AssistantVocabulary` touches a criterion I checked: it touches **none of the six**. It is read only through `GetVocabulary()` → `GetCachedZoneA()` (`SiegeAssistantComponent.cpp:2690`, `:2646`), which feeds Zone A. A null vocabulary is remembered as a result (`bVocabularyResolved` is set *before* the load, `:2702`), warns once (`:2709-2715`), and leaves Zone A byte-stable — so it degrades prompt content, not the FSM, the table, the short-circuit, the latch or the authority gate. **Not chased further.**

---

## Findings

### [WARN-1] `SiegeAssistantComponent.cpp:944` (`MarkAssistantFaulted`) — the session fault latch has **ZERO callers**, so `bAssistantFaulted` can never become true

`MarkAssistantFaulted` is the **only** writer of `bAssistantFaulted` (`:951` is the sole `= true` in the pair). Nothing in the repo calls it.

- **Search discipline (§14), because a negative alone proves nothing here.** `MarkAssistantFaulted` appears at exactly 7 sites across `*.{h,cpp,cs,py}`: `SiegeAssistantComponent.h:832` (declaration), `.cpp:944` (definition), and **five PROSE MENTIONS INSIDE COMMENTS** — `.cpp:1376`, `:1895`, `:1976`, `:2048`, `:2292` (this is §14 instance 2 exactly: *a match ≠ a call site*). **Zero call sites.**
- **Positive control, run in the same tool over the same glob:** `SubmitUtterance` and `NotifyConsoleOpened` returned real cross-file call sites (`SiegeAssistantComponent.cpp:2172` `AddDynamic`, `:2240` direct call; plus `SiegePlayerController.{h,cpp}` hits). The search domain reaches the callers it should. The files are tracked as of `cd5f4ed`, so §14 instance 3 (untracked blindness) does not apply either.

**Consequences, traced:** `IsAssistantFaulted()` always returns false · `IsConsoleAvailable()` always returns true · `OnAssistantAvailabilityChanged` is **never broadcast false**, so the widget's `SetConsoleEnabled(false, …)` path is dead · `SubmitUtterance` gate 2 (`:649`) and `NotifyConsoleOpened`'s faulted branch (`:567`) are unreachable · `PollDeferredIntent`'s `bAssistantFaulted` belt (`:1893`) is unreachable.

**Why this is a WARN and not a BLOCKER — stated so the severity is arguable rather than asserted.** Criterion (5)'s ⛔ **hard** half is *"a model-load failure, GGML fault, timeout or missing GGUF must NEVER block match start or degrade any key."* That half **PASSES**, and the inertness runs in the *safe* direction: a latch that cannot fire cannot disable anything. What is lost is the *soft* half — a genuinely dead backend produces one refusal line per sentence forever instead of one Error line and a disabled console. No crash, no key degraded, no engine risk.

**Suggested fix — ⚠️ HYPOTHESIS, NOT A PATCH (§20; the implementer traces it before typing it):** the natural caller is `DispatchTurnToModel` / `HandleModelCompletion`, but **both were deliberately written NOT to latch** (`:1098-1117` argues at length that `!IsReady()` must refuse the TURN and not the session, and that argument is correct). ⇒ **The fix is not "call it from there."** It needs a manager ruling on *what condition constitutes a session fault* first — this is a design gap, not a missing line.

---

### [WARN-2] `SiegeAssistantComponent.cpp:401` — `ESiegeAssistantReasonCode::FailedTimeout` is authored but **unreachable**; every hard timeout is reported to the player as `FailedModelError`

`EnterFailed` is called from exactly three sites — `:840` (`FailedModelError`), `:861` (`FailedParse`), `:941` (`FailedParse`). `FailedTimeout` is produced by nothing. Its row (*"The assistant took too long."*) can never be shown; the plugin's 10 s hard timeout (`SiegeLlamaSubsystem.h:161`) arrives as `bSuccess=false` and lands on `FailedModelError` (*"The assistant could not answer."*) at `:835-841`.

**Root cause, verified at the artifact:** `FSiegeLlamaCompletionSignature` is `DECLARE_DELEGATE_ThreeParams(…, bool, const FString& Output, const FString& Error)` (`Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSubsystem.h:29`) — **there is no structured error kind**, so the FSM cannot distinguish a timeout from a backend error without string-matching `Error`. ⇒ **The fix is not one line**: it is either a pinned-§9 signature change (manager ruling) or fragile text matching. **Untraced (§20).**

---

### [WARN-3] `SiegeAssistantComponent.h:141` · `.cpp:526` · **`.cpp:535` (a runtime `UE_LOG`)** — three artifacts assert *"holds no reference into the plugin"*, and all three are now FALSE

Swept for the **claim**, not for the comment (§22 — *"searching for the comment finds the comment; searching for the claim finds the logs too"*). Three sites, and **the worst one is the log**:

| # | site | text (read from raw `Read` output, not `Grep`) |
|---|---|---|
| 1 | `SiegeAssistantComponent.h:141` | *"This component never ticks, never blocks, and holds no reference into the plugin."* |
| 2 | `SiegeAssistantComponent.cpp:526` | *"this component holds no reference into the SiegeLlama plugin at all"* |
| 3 | ⛔ **`SiegeAssistantComponent.cpp:535` — `UE_LOG(…, Log, …)`, printed at EVERY `BeginPlay`** | *"This component NEVER ticks and holds no plugin reference; a model fault can only ever disable the console."* |

It **does** hold one, as shipped: `#include "SiegeLlamaSubsystem.h"` (`.cpp:28`), `class USiegeLlamaSubsystem;` (`.h:20`), `ResolveLlamaSubsystem()` (`.cpp:2094`), `DispatchTurnToModel` (`.cpp:1093`), `EnsureStaticPrefixRegistered` (`.cpp:2752`), and `+ "SiegeLlama"` in `GitClaudeUnrealTest.Build.cs`.

**Why this is a finding and not pedantry, and why it is criterion (5):**
- The **conclusion** the claims support — *"a missing GGUF never blocks match start"* — is **still TRUE**: `BeginPlay` (`:519-538`) calls `EnsureSnapshot` + `GetCachedZoneA` and touches nothing in the plugin. ✅ Criterion (5)'s match-start half PASSES **on the code**.
- But the claims assert it is **structural**, and that mechanism is gone. It is now a property of *"`BeginPlay` happens not to call `ResolveLlamaSubsystem`"* — one edit away, enforced by nothing.
- ⛔ **Line 535 is evidence-shaped and it is now false in BOTH clauses**: with WARN-1, *"a model fault can only ever disable the console"* is also untrue — it cannot even do that. This is §22's exact ledger entry: *a `UE_LOG(…)` misleads the next debugger, at 3 a.m., during a playtest, in a file Jonathan greps* — and it will be believed over the code.

**Sweep result:** those three are the whole extent of this claim's shape in the assistant lane. **A sweep that finds nothing further is a result, and this one bottomed out at three.**

---

### [WARN-4] `SiegeAssistantComponent.cpp:360` — the `ShortfallCount` template hard-codes the verb **"Send"** for a path that also serves **Guard** and **Ambush**

```
"You asked for {Requested} {Kind} - {Available} can take that order. Send {Available}?"
```

`FindShortfall` runs for exactly Send / Guard / Ambush (`:2434-2439`). So *"guard the mine with 10 footmen"* against 8 orderable prints **"…Send 8?"**, naming a **different shipped intent** — `Send` and `Guard` are distinct `ESiegeAssistantIntent` members with distinct display words (`:138-140`). The player answering *"yes"* then gets a **Guard**, which is what they asked for but not what they were just asked to confirm.

This is squarely criterion (3) (*the game authors every player-facing sentence*) and it is the one place the template table says something untrue about the order. It matters most with the confirm toggle **OFF**, where this sentence is the only description the player sees before execution. Cheap fix (a verb-neutral phrasing, or `{Order}`); **not applied — I never edit code.**

---

### [WARN-5] `SiegeAssistantComponent.h:132` vs `.cpp:138-144` + `.cpp:1067,1071,1075` — the file violates its own stated §3 rule, and the handoff's completeness claim overstates the table

The header states the rule: *"A player-facing literal appearing anywhere but `SiegeAssistantReasonTemplate` is a §3 violation"* (`.h:132`). Handoff §5 states the claim: *"Every sentence the player reads comes from `SiegeAssistantReasonTemplate` (20 rows) or `SiegeAssistantStateLabel`."*

**Counted, not eyeballed — all 34 `NSLOCTEXT` sites in the `.cpp`:**

| where | count |
|---|---|
| `SiegeAssistantReasonTemplate` | **19** |
| `SiegeAssistantStateLabel` | 5 |
| ⚠️ `SiegeAssistantComponentInternal::IntentDisplayText` (`:138-144`) | **7** |
| ⚠️ `DescribeCommandForPlayer` sentence frames (`:1067`, `:1071`, `:1075`) | **3** |

(A 35th `NSLOCTEXT` token at `:83` is a comment, not a site.)

The player reads all ten of the flagged strings — they arrive inside `{Order}` in `ConfirmPrompt` / `Executed` / `DeferredArmed`. **The substance of §3 HOLDS**: every one is game-authored, localizable, function-local, and built from a struct pinned to `uint8`/`int32`/`FName` — **no model-produced text reaches the player anywhere in this file** (see the criterion-3 verification below). What fails is the **invariant as stated**: a future reviewer or localization pass auditing §3 by inspecting the one named function would report a false clean over ten real strings. Fix is a ruling — either move the ten into the table, or amend `.h:132` and the handoff to name all four sites.

---

### [WARN-6] `SiegeAssistantComponent.cpp:2310` (`TryShortCircuitClarification`) — the short-circuit answers from the **previous sentence's** snapshot, which can be arbitrarily old

The short-circuit runs at **gate 5**, before `BeginTurn` (gate 6) and before `CaptureTurnSnapshot` (gate 7) — so `FindShortfall`'s re-run (`:2384`) and the whole accepted path read the `Snapshot` captured for the *previous* utterance. A player can sit in `Clarify` indefinitely; units die in the meantime. Downstream, `RouteParsedCommandInternal`'s guard and place-eligibility steps also ride that same stale snapshot.

**It fails safe, and that is why this is a WARN and not a BLOCKER:** the executor selects against a **live** `TActorIterator` (`:1676`) and, per its documented contract, **refuses rather than truncating** on shortfall — so a stale-arithmetic acceptance surfaces as a refusal, never as a silently reduced order. (Cited as mitigation only; the executor is 446-gated and I did not review it.)

**Suggested fix — ⚠️ UNTRACED HYPOTHESIS (§20).** A `CaptureTurnSnapshot()` inside the short-circuit costs **zero model calls** (~0.2 ms of iteration), so it does not violate §4 or the "zero model calls" property. **But it changes which snapshot the guard and the place resolution see, and it is a second `Capture()` in a window the header explicitly reasons about** (`.h:842-844`, `.cpp:1937-1944`). ⛔ **Do not apply this without tracing it against §1's *"one utterance + one snapshot"* and against the routing path.**

---

### [NIT-1] `SiegeAssistantComponent.cpp:907` + `:935` — an `{"ask":"unsupported"}` decline parks the FSM in `Clarify`, labelled *"Waiting for your answer"*

`ClarifyCode` defaults to `AskUnsupported` (`:907`) and `EnterClarify` is called unconditionally (`:935`), so a **refusal** enters the state whose documented meaning is *"the game is holding a question"* (`.h:101`) and whose player label is *"Waiting for your answer"* (`:474`). Nothing was asked. **Harmless in every direction I traced:** `BuildProblemClause` deliberately returns empty for `AskUnsupported` (`:173-175`), so Zone C prints `pending: none` and **no false context is carried**; and the next utterance falls out of the short-circuit at `:2326` (`PendingReason != ShortfallCount`) into an honest fresh single-turn call, so **no turn is swallowed**.

### [NIT-2] `handoffs/TASK-442-programmer.md` §5 and the board's TASK-442 status line say **"20 rows"**; the table has **19**

All 19 non-`None` codes have a row; `None` falls to the empty default at `:438-446` by design. ⇒ **Coverage is COMPLETE**, which is the property that matters; the count is off by one. Recorded under §22 (*a count in a spec is a hint, never a contract; a mismatch is a finding about the spec, not a defect in the work*).

### [NIT-3] `SiegeAssistantComponent.h:951` · `.cpp:1376` · `:2048` — *"`MarkAssistantFaulted` … calls it unconditionally"* is inaccurate

`MarkAssistantFaulted` calls `AbortInFlightRequest()` only `if (State == Thinking)` (`:959`) and `ClearConfirmPreview()` only `if (State == AwaitConfirm)` (`:963`). **Behaviourally equivalent** — I swept every exit from `AwaitConfirm` (`ConfirmPressed :761` · `CancelPressed :786` · `NotifyConsoleClosed :619` · `MarkAssistantFaulted :965` · `EndPlay :552`) and confirmed `bConfirmPromptUp` ⟺ `State == AwaitConfirm`, so the conditional can never skip a live preview. Wording only.

---

## What I VERIFIED, per TASK-464's numbered criteria

**(1) THE CENTRAL LAW — CONFIRMED, not re-derived.** ⚠️ **I am CONFIRMING TASK-446's criterion (b), which already checked this file-wide.** The four structural mechanisms hold at the current file, each checked by symbol:
1. **No member holds model output.** `Output` occurs at exactly `:815` (parameter), `:849` (argument to `ParseSiegeAssistantCommand`), `:1173`/`:1177` (lambda parameter → forwarded). **Never assigned to a member.** `Error` occurs only at `:839` (logged) and `:1177` — **never shown to the player.** ✅
2. **The FSM composes; the dispatcher cannot.** `ComposeTurnPrompt` is private (`.h:1166`), called only from `SubmitUtterance:722`; `DispatchTurnToModel(:1093)` receives finished strings and reaches no zone builder. ✅
3. **`BuildPendingLine()` takes no parameters** (`.cpp:2485`, `.h:1154`); reads only `PendingArgs` + `PendingReason`. ✅
4. **One dispatch per turn.** `TurnId` is mutated at exactly **one** site in the whole file — `++TurnId` at `:2277` inside `BeginTurn`. The stale-completion guard is `:821`; the state guard is `:828`. ✅

**(2) THE FSM STATES + `uint8` PUSH — PASS.** All 7 states declared (`.h:300-323`). **I swept all 14 `SetState` call sites** rather than sampling: `:577` `:627` `:691` `:714` `:737` `:746` `:807` `:1219` `:1246` `:1327` `:1346` `:1804` `:1861` `:1916` `:1951` `:2295` `:2306` `:2345`. Every documented edge is present and **no undocumented edge exists**. The pinned admission table (`.h:114-119`) matches `SubmitUtterance`'s switch (`:669-697`) row for row: Thinking→`RefusedBusy`, AwaitConfirm→`RefusedAwaitingConfirm`, Deferred→latch cancelled then a fresh turn, everything else→a new turn. The pinned 9-step gate order (`.h:760-772`) is implemented **in that exact order** at `:641` `:649` `:659` `:669` `:700` `:706` `:709` `:722` `:726` — including empty-input **first**, with a `Verbose` log and **zero side effects**. State crosses as `static_cast<uint8>(State)` (`:2269`), the delegate is declared `uint8` (`.h:493`), and the one enum-typed accessor `GetState()` (`.h:644`) carries **no `UFUNCTION`**. ✅

**(3) THE REASON-CODE TEMPLATE TABLE — PASS on the law, WARN-4/WARN-5 on the table.** **Swept every enum member against its table:** all 19 non-`None` `ESiegeAssistantReasonCode` members have an authored row (`:322-448`); all 7 `ESiegeAssistantState` members are covered by `SiegeAssistantStateLabel` (`:450-494`), with `Idle`/`Failed` blank by declared design. `PushMessage` refuses to print an empty template and logs loudly instead (`:990-998`) — silent to the player, loud in the log, which is the right way round. ⛔ **No model-produced text reaches the player anywhere:** the model's `Error` is logged only (`:839`), its `Output` dies in a local, and the only model-derived thing in a sentence is a **canonical symbol** the grammar already constrained.

**(4) THE SHORT-CIRCUIT — PASS; it cannot silently swallow a turn.** `TryShortCircuitClarification` (`:2310`) is gated on `State == Clarify` **and** `PendingReason == ShortfallCount` **and** a valid `PendingShortfallIndex`. **I traced every `return true` path** — negative (`:2341-2348`, pushes `Cancelled`), re-ask (`:2360-2370`, pushes a fresh `ShortfallCount`), accepted-with-another-short-kind (`:2384-2398`, pushes again), accepted-clean (`:2400-2416`, routes the order) — **each one either shows the player a game-authored sentence or hands the command onward. None returns silently.** Every other path returns `false` and costs one honest model call. The classifier (`:235-316`) is exact-match-only after trim/lower/trailing-punctuation strip (`"no archers"` cannot match `"no"`), takes **bare integers only**, and the **length guard precedes `Atoi64`** (`:275-278` before `:282`) with the range read from the grammar (`GrammarCountMin = 1`, `GrammarCountMax = 30`, `SiegeAssistantGrammar.h:49`/`:70`) rather than re-copied. The multi-kind re-run **terminates**: `FindShortfall` returns the *first* short index, so each settled index strictly advances. ⚠️ The declared "~15 lines → ~45 + ~40" size deviation is **RATIFIED under §15**, not filed: it was declared, the mechanism was named (negative branch, re-ask, multi-kind re-run, overflow guard), and each block is load-bearing. WARN-6 is the one thing this criterion turned up.

**(5) THE FAULT POSTURE — the ⛔ hard half PASSES; the latch itself is INERT (WARN-1, WARN-3).** `MarkAssistantFaulted`'s body (`:944-981`) touches **only** assistant-owned state: `AbortInFlightRequest`, `ClearConfirmPreview` (destroys its own two `ADecalActor` members and nothing else), `ClearDeferredIntent`, `ClearPendingIntent`, `SetState`, `PushMessage`, and one `OnAssistantAvailabilityChanged` broadcast. **No key, no card, no stance, no group, no `UnitGroups` touch.** Match start cannot be blocked: `BeginPlay` (`:519-538`) reaches no plugin symbol. Logged **once**, at `Error`, and the line states its own blast radius.

**(6) AUTHORITY — PASS, verified at BOTH artifacts rather than from the comment.** `SubmitUtterance` gate 3 refuses on `!OwningActor || !OwningActor->HasAuthority()` (`:659-666`) — the keys' own predicate plus a null-owner guard, i.e. **never more capable, marginally more conservative**. The wording is identical **character-for-character, down to namespace and key** (both quoted from raw `Read` output):
- assistant — `SiegeAssistantComponent.cpp:373`: `NSLOCTEXT("Siegebound", "Refused_OnlineObserver", "Not available yet in online matches")`
- keys — `SiegePlayerController.cpp:59` (`GetObserverLockoutText`): `NSLOCTEXT("Siegebound", "Refused_OnlineObserver", "Not available yet in online matches")`

The controller applies it behind `if (!HasAuthority())` at `PlayHandSlot` (`:684-690`) and `DiscardHandSlot` (`:846-852`) — **the same predicate**. They dedupe to one localization entry and cannot drift. **Sweep:** every route to an execution originates at `SubmitUtterance`, so no order can reach the ground without passing this gate. ✅

**(7) THE M8 DECLARATION — PASS, verbatim, and verified against the CODE.** `handoffs/TASK-442-programmer.md:10` states it exactly: ***"adds no replicated property, no new replicated class, no new relevancy tier."*** `SiegeAssistantComponent.h:28-32` repeats it verbatim in the class comment. **Checked, not assumed:** `Replicat` matches **zero** times in either file — no `UPROPERTY(Replicated)`, no `GetLifetimeReplicatedProps`, no `SetIsReplicatedByDefault`. **Positive control:** the same pattern matches **49 times across 15 other `Siegebound` files** (`Castle.h`, `SiegeGameState.h`, `SiegePlayerState.h`, …), so the negative is about the file and not about the search. ✅

**(8) NO UNIT REGISTRY / ACTOR CACHE / DIRTY FLAG / SUBSCRIPTION LIST — PASS.** **Swept every member** in `.h:1246-1416`: no unit container anywhere. `Snapshot` is **one** object created once (`:2569-2578`) and re-`Capture()`d per sentence (`:2599`). The two `TActorIterator` uses (`:1676`, `:2018`) are on-demand and keep nothing. `CachedZoneA` caches the **static prompt prefix**, which §8 *requires* to be byte-identical — not a state cache. **Every `AddDynamic` in the file (9 of them, `:2172-2185`) is a console-widget forward**; there is **no subscription to any spawn/death/gameplay event**, i.e. no subscription list in §4's sense. The component **never ticks** (`:505-506`), and no `SetComponentTickEnabled` exists anywhere in the pair. ✅

---

## ⛔ What I DELIBERATELY DID NOT REVIEW, and why

Ruled out of scope by TASK-464's spec and by CONVENTIONS §27/§29 — **all of it was gated and PASSED at TASK-446 (`qa/TASK-446.md`, 0 blockers) or TASK-462**, and a retroactive gate that re-litigates a passed review *"is not being thorough; it is being unable to stop."*

1. **The confirm toggle and its OFF path** — `IsConfirmBeforeExecuteEnabled` (`:2079`) and its single call site. Read only far enough to confirm it is not entangled with the six criteria above. **446's discharged off-path trace stands untouched.**
2. **The non-orderable-kind guard** — `RouteParsedCommandInternal` step 1.
3. **The executor** — `ExecutePendingCommand`, `SelectUnitsForOrder`, `ExecuteZoneOrder`, `ExecuteFollowOrder`, `ExecuteRallyOrder`. Cited **once**, as WARN-6's downstream mitigation, on its documented refuse-never-truncate contract — cited, not reviewed.
4. **The confirm step** — `EnterAwaitConfirm`, `ClearConfirmPreview`, `SpawnConfirmPreview`, and the two TASK-443 confirm delegates. Read only to complete the FSM edge sweep (criterion 2) and to settle NIT-3; **no verdict offered on their policy.**
5. **The deferred intent** — `EnterDeferredIntent`, `PollDeferredIntent`, `ClearDeferredIntent`, the TTL/1 Hz timer, and the DEV-11 re-ask ruling. Read only for its `SetState` edges.
6. 📌 **TASK-443's statement-for-statement-identical edit to 442's own `ConfirmPressed` (`:751-769`)** — **already ruled at 446 and NOT re-opened**, exactly as instructed.
7. **`DA_AssistantVocabulary`'s non-resolution** — TASK-463's, noted above only to record that it touches none of my six criteria.
8. **TASK-455's static-prefix wiring** (`EnsureStaticPrefixRegistered`) and the three `static_assert`s (`:52`, `:59`, `:71`) — 456/446 territory; read only to establish that the plugin reference exists, which is WARN-3's basis.

I also did **not** run, compile, launch the editor, enter PIE, or execute anything. **This report is a source review.**

---

## Notes for build-master

- ✅ **This gate discharges the CONVENTIONS §29 coverage-ledger debt for TASK-442.** The breach is recorded as a breach, the commit stands, and the remediation is this report. **`cd5f4ed` needs no follow-up commit on account of TASK-464** — there is nothing to build here.
- ⛔ **Six WARNs and three NITs are OPEN and belong to the manager's disposition, not to a revert and not to a silent close.** In dependency order, the two that most want a fix task:
  1. **WARN-1 + WARN-2 together are one design gap** — *what constitutes a session fault, and can the FSM see it through the pinned §9 surface?* They need a **manager ruling before any code**, because both plausible fixes touch a pinned registry or contradict `DispatchTurnToModel`'s own (correct) reasoning.
  2. **WARN-3 is the cheapest and the most urgent to a human** — a `UE_LOG` printing a false statement into the log at every `BeginPlay`, false in **both** clauses. It is three artifacts, one of which is the log; §22 says sweep all three in one pass.
- ⚠️ **WARN-4 (`"Send {Available}?"` on a Guard/Ambush order) is the only finding a playtester can see without reading a log.** If TASK-448 is imminent, it is worth fixing before Jonathan sits down, so a wording defect is not reported back as a routing bug.
- ⚠️ **Nothing in this report is accuracy progress, and none of it moves bar #5** — 20/25 vs 22 stands uncleared. This gate reviewed what the game *does*, not what the model *emits*.

## Board flips to apply (⛔ I do not edit the board — the orchestrator applies these)

- **TASK-464** → `done` — *PASS on TASK-442, 0 blockers, 6 WARN, 3 NIT; report `qa/TASK-464.md`. The §29 retroactive gate is discharged and the breach is on the record.*
- **TASK-442** → `qa-passed` — *retroactively, by `qa/TASK-464.md` (TASK-464), scoped to 442's own criteria. ⚠️ The code was already committed in `cd5f4ed`; this closes the coverage gap, it does not re-open the commit.*
- **Recommended NEW task (manager's call):** the WARN-1 + WARN-2 fault-path ruling, and the WARN-3 §22 three-artifact sweep. **Not opened by me — QA does not create tasks.**
