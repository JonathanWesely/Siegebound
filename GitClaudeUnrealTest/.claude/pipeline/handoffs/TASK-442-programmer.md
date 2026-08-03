# TASK-442 — [W1-B2a] `USiegeAssistantComponent`: the FSM skeleton, reason codes + template table, and the clarification short-circuit

**Assignee:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-08-03
**Files (the ONLY two touched — both NEW):**
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantComponent.h`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantComponent.cpp`

**Not touched, deliberately:** no controller edit, no snapshot edit, no grammar edit, no `Build.cs`, no `Content/`, no `.ini`, no Git, **no compile**, no editor, no MCP, no PIE.

**M8 DECLARATION DUTY, verbatim:** *"adds no replicated property, no new replicated class, no new relevancy tier."*
Stated in the header's class comment too. The reason: this component is client-local FSM state that governs a LOCAL review step and a LOCAL prompt; the ORDERS it will eventually produce (TASK-443) go through the same authoritative controller APIs the keys use, and the component itself refuses on `!HasAuthority()`.

---

## 1. What was built

A complete, finished FSM with **named, empty, safely-refusing seams** for TASK-443. Everything §1 calls "the FSM owns the conversation" is here; everything §1 calls "the executor guarantees legality" is a seam.

### The states (task spec §2, exactly)

`Idle → Composing → Thinking → { AwaitConfirm | Clarify | Failed }` **plus** `Deferred`.

| state | meaning | edges out |
|---|---|---|
| `Idle` | console shut, nothing pending | `NotifyConsoleOpened` → `Composing` |
| `Composing` | console open, waiting for a sentence | `SubmitUtterance` → `Thinking`, or a refusal that leaves the state where it was |
| `Thinking` | **exactly one** model request in flight (queue depth 1) | `HandleModelCompletion` → `AwaitConfirm`/`Clarify`/`Failed`; `CancelPressed`; `NotifyConsoleClosed` |
| `AwaitConfirm` | a parsed, guard-passed order + ghost circles (443's body) | `ConfirmPressed` → execute → `Composing`/`Idle`; `CancelPressed` → discard |
| `Clarify` | the game is holding a question; the pending line carries it forward | next utterance → short-circuit **or** a fresh single-turn call |
| `Failed` | **this turn** failed. ⚠️ NOT the session latch | next utterance starts a clean turn |
| `Deferred` | a deferred intent is latched (443's timer). **Survives the console closing** | trigger/TTL (443); a new utterance drops the latch; `CancelPressed` |

**State is pushed to the widget as a `uint8`, never an enum** — `FOnAssistantStateChanged(uint8 NewState, const FString& StateLabel)`.

### The admission table for `SubmitUtterance` (pinned in the header so 443 does not infer it)

| state | result |
|---|---|
| `Idle` / `Composing` / `Clarify` / `Failed` | a new turn starts |
| `Thinking` | **REFUSED**, `RefusedBusy` (queue depth 1 is the design) |
| `AwaitConfirm` | **REFUSED**, `RefusedAwaitingConfirm` |
| `Deferred` | the latch is **cancelled** (`DeferredCancelled`) and a new turn starts |

### The pinned gate order inside `SubmitUtterance`

1. **empty input** — ignored with **zero side effects**
2. fault latch (§2)
3. **AUTHORITY** (§7)
4. admission (table above)
5. **the short-circuit** — may finish the turn with **zero model calls**
6. `BeginTurn()` — the only place `TurnId` moves
7. the snapshot — **once per SENTENCE, never per tick**
8. compose prompt + grammar — **the FSM's, never the executor's**
9. dispatch — the TASK-443 seam

⚠️ **Empty-input is step 1 on purpose, and it is a real bug I caught in my own draft:** it started at step 4, *after* the `Deferred` admission — so pressing Enter on an empty box would have silently dropped a latched order. An empty submission must never have a side effect.

⚠️ **None of steps 1–8 is skippable by the confirm toggle.** The toggle is read in TASK-443's `RouteParsedCommand` and nowhere near this function.

---

## 2. How §1 is made STRUCTURALLY true, not merely observed

> *One utterance + one snapshot → one constrained JSON; the game owns the dialogue; a multi-turn model loop is a QA FAIL.*

Four mechanisms. Each is something a later edit would have to **delete**, not merely overlook — that is the difference between a rule and a comment, and it is the part I would most like QA to attack:

1. **No member of this class ever holds model output.** The raw JSON is a function local in `HandleModelCompletion` and dies with the frame. The only thing that survives a turn is `FSiegeAssistantMessageArgs`, whose **every field is `uint8`/`int32`/`FName`** (`FSiegeAssistantCommand` is itself pinned to those three by §9). ⇒ There is nowhere to put a transcript; "append the last reply" is a new member and a new type, not a small edit.
2. **The FSM composes the prompt — not the executor and not the model call.** `ComposeTurnPrompt()` is **private** and assembles Zone A + Zone B + Zone C itself; the seam `DispatchTurnToModel(Prompt, Grammar)` receives a **finished** string and has no route to a zone builder. ⇒ The function that could feed the model its own words cannot reach the model, and the one that reaches the model cannot compose.
3. **The pending line is built from typed symbols only.** `BuildPendingLine()` takes **no parameters** and reads only `PendingArgs` (`uint8`/`int32`/`FName`) and `PendingReason` (an enum). No `FString` in its signature ⇒ model text cannot be routed into it without changing that signature.
4. **One dispatch per turn, enforced by a counter.** `TurnId` increments in `BeginTurn()` **and nowhere else**; `bModelDispatchedThisTurn` refuses a second dispatch inside a turn; and `HandleModelCompletion` **drops** any completion whose `TurnId` is not current. A late answer to a superseded sentence can never reach the executor, so two model outputs can never meet.

---

## 3. ⛔ THE EXACT SEAMS TASK-443 FILLS

All eight are **private methods on `USiegeAssistantComponent`**, already declared, defined, and called from the finished FSM. **443 replaces the bodies in place** — it does not add a parallel set, and it must not change a name or a signature.

| seam | signature | what 443 puts in it |
|---|---|---|
| model call | `bool DispatchTurnToModel(const FString& Prompt, const FString& Grammar)` | `Build.cs` += **`SiegeLlama` only**; resolve `USiegeLlamaSubsystem`; `IsReady()`/`IsBusy()`; `RequestCompletion(Prompt, Grammar, OnComplete)`. `false` ⇒ FSM refuses the turn. **The prompt and grammar arrive finished.** |
| routing | `void RouteParsedCommand(const FSiegeAssistantCommand& Command)` | ① `ValidateCommandAgainstSnapshot` → `PushMessage(AskUnsupported)` on reject ② eligibility ③ `TriggerKind != NAME_None` → `EnterDeferredIntent()` ④ `IsAssistantConfirmEnabled()` ON/unresolvable → `EnterAwaitConfirm()`, OFF → `ExecutePendingCommand()` |
| confirm up | `void EnterAwaitConfirm()` | `PushMessage(ConfirmPrompt, PendingArgs)` + `SpawnGroupCircleDecal` ghosts + `SetState(AwaitConfirm)` |
| confirm down | `void ClearConfirmPreview()` | tear the ghosts down. **Must be safe when nothing is up** — `EndPlay`, `CancelPressed`, `NotifyConsoleClosed`, `ConfirmPressed` and `MarkAssistantFaulted` all call it unconditionally |
| execute | `bool ExecutePendingCommand()` | `CreateUnitGroup` / `EnrollInDefaultFollowGroup` / `SetUnitCommand` / `Rally` — **the same public APIs the keys call**. `false` ⇒ FSM says `AskUnsupported` |
| defer arm | `void EnterDeferredIntent()` | latch + **120 s TTL** + **1 Hz timer armed only while latched** + re-resolve → `AwaitConfirm` |
| defer drop | `void ClearDeferredIntent()` | drop latch + stop timer. **Must be safe when nothing is latched** (four unconditional callers) |
| abort | `void AbortInFlightRequest()` | `CancelActiveRequest()`. **Must be safe when nothing is in flight** |

**Every stub logs at Warning and refuses safely.** A build that somehow reached the editor without 443 would parse, clarify and refuse, and would execute **nothing**. That posture is chosen: a stub that quietly returned success would ship an assistant claiming to have given an order it never gave — the valid-shaped-wrong-command failure wearing the implementer's clothes.

### ⚠️ Caller contract 443 must not break
`RouteParsedCommand`'s `Command` argument is **always a local copy**, never a reference into `PendingArgs`. Both existing call sites honour it. 443's body is therefore free to reset the pending state mid-body without its own argument changing underneath it. (I hit this in draft: the short-circuit originally passed `PendingArgs.Command` by reference into a stub that then called `ClearPendingIntent()`.)

### The exact model-completion binding 443 should write
```cpp
const int32 DispatchedTurnId = GetTurnId();
FSiegeLlamaCompletionSignature OnDone;
OnDone.BindWeakLambda(this,
    [this, DispatchedTurnId](bool bSuccess, const FString& Output, const FString& Error)
    {
        HandleModelCompletion(DispatchedTurnId, bSuccess, Output, Error);
    });
```
`FSiegeLlamaCompletionSignature` is a plain (non-dynamic) delegate, so the turn id rides in the capture. `HandleModelCompletion` is public and is **not** a `UFUNCTION` for that reason.

---

## 4. Published signatures (what other tasks may compile against)

```cpp
// SiegeAssistantComponent.h — file scope
UENUM() enum class ESiegeAssistantState : uint8
{ Idle=0, Composing=1, Thinking=2, AwaitConfirm=3, Clarify=4, Failed=5, Deferred=6 };

UENUM() enum class ESiegeAssistantReasonCode : uint8
{ None=0, AskWhichUnit=1, AskWhichPlace=2, AskHowMany=3, AskWhichIntent=4, AskUnsupported=5,
  ShortfallCount=6, RefusedNoAuthority=7, RefusedAssistantUnavailable=8, RefusedBusy=9,
  RefusedAwaitingConfirm=10, FailedParse=11, FailedModelError=12, FailedTimeout=13,
  ConfirmPrompt=14, Cancelled=15, Executed=16, DeferredArmed=17, DeferredExpired=18,
  DeferredCancelled=19 };

enum class ESiegeAssistantClarifyReply : uint8 { Unrecognized=0, Affirmative=1, Negative=2, Quantity=3 };

bool         SiegeAssistantParseClarificationReply(const FString& Reply, ESiegeAssistantClarifyReply& OutKind, int32& OutQuantity);
const FText& SiegeAssistantReasonTemplate(ESiegeAssistantReasonCode Code);
const FText& SiegeAssistantStateLabel(ESiegeAssistantState InState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssistantStateChanged, uint8, NewState, const FString&, StateLabel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FOnAssistantMessage, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssistantAvailabilityChanged, bool, bAvailable, const FString&, Reason);

USTRUCT() struct FSiegeAssistantMessageArgs   // ⛔ uint8/int32/FName ONLY — no FString, no FText
{ FSiegeAssistantCommand Command; int32 Requested=0; int32 Available=0; FName Kind=NAME_None; };

// USiegeAssistantComponent : UActorComponent — public
uint8    GetStateAsByte() const;                    // UFUNCTION(BlueprintPure)
FString  GetStateLabel() const;                     // UFUNCTION(BlueprintPure)
ESiegeAssistantState GetState() const;              // C++ only (widget-param law)
FString  GetLastMessage() const;                    // UFUNCTION(BlueprintPure)
bool     IsAssistantFaulted() const;                // UFUNCTION(BlueprintPure)
bool     IsConsoleAvailable() const;                // UFUNCTION(BlueprintPure)
void     NotifyConsoleOpened();                     // UFUNCTION(BlueprintCallable)
void     NotifyConsoleClosed();                     // UFUNCTION(BlueprintCallable)
void     SubmitUtterance(const FString& RawUtterance);  // UFUNCTION(BlueprintCallable)
void     ConfirmPressed();                          // UFUNCTION(BlueprintCallable)
void     CancelPressed();                           // UFUNCTION(BlueprintCallable)
void     HandleModelCompletion(int32 InTurnId, bool bSuccess, const FString& Output, const FString& Error);
void     MarkAssistantFaulted(ESiegeAssistantReasonCode Reason);
const USiegeAssistantSnapshot*  GetTurnSnapshot() const;
const FSiegeAssistantCommand&   GetPendingCommand() const;
int32    GetTurnId() const;
void     PushMessage(ESiegeAssistantReasonCode Code, const FSiegeAssistantMessageArgs& Args);
void     PushMessage(ESiegeAssistantReasonCode Code);
FText    DescribeCommandForPlayer(const FSiegeAssistantCommand& Command) const;
```

**Consumed from the pinned registries, unchanged:** `FSiegeAssistantCommand` · `ESiegeAssistantIntent` · `ParseSiegeAssistantCommand` · `SiegeAssistantValidateSelection` · `SiegeAssistantReasonCode` · `SiegeAssistantIntentToSymbol` · `SiegeAssistantReason::*` · `SiegeAssistantAsk::*` · `LogSiegeAssistant` · `USiegeAssistantSnapshot::{Capture, BuildZoneA, BuildZoneB, BuildZoneC, GetRoster, GetUnitKinds, GetPlaceNames, GetOrderableCount, MaxSnapshotChars, MaxRosterKinds}` · `USiegeAssistantGrammar::{Build, GrammarCountMin, GrammarCountMax}` · `ASiegePlayerState::GetTeam`. **Nothing pinned was renamed, widened or "improved."**

---

## 5. §3 — this file is the only place player text exists

Every sentence the player reads comes from `SiegeAssistantReasonTemplate` (20 rows) or `SiegeAssistantStateLabel` (5 labels + 2 deliberate blanks), filled through `PushMessage`. **⇒ TASK-443 authors no strings:** the confirm summary it needs is `DescribeCommandForPlayer()`, and every other outcome is `PushMessage(Code, Args)`.

- **`RefusedNoAuthority` reuses the keys' approved wording character-for-character**, down to namespace and key: `NSLOCTEXT("Siegebound", "Refused_OnlineObserver", "Not available yet in online matches")` — identical to `ASiegePlayerController`'s `GetObserverLockoutText()`. They dedupe into one localization entry and cannot drift. The controller's copy is a translation-unit-local static and is deliberately not exported; the shared key is the grep that ties them.
- **The non-orderable-kind guard gets NO new player surface** (§6). There is deliberately **no reason code** for "that kind cannot take orders" — 443's guard rejection, a refused execution and the model's own `{"ask":"unsupported"}` all land on `AskUnsupported`.
- **Two text domains are kept apart in the file, with opposite rules:** player text = `FText`/`NSLOCTEXT`, built inside functions (never module static-init — the `GetObserverLockoutText` precedent). Prompt text = the pending line only: **ASCII, terse, byte-counted.**
- ⚠️ **§1 illustrates the pending line with `×` (U+00D7). I write `x`.** The doc's glyph is prose; this one is a byte inside Zone C, where the ASCII law at the head of `SiegeAssistantSnapshot.cpp` binds. Deviation is deliberate and commented at the site.
- The only model-derived content that reaches a player-facing string is a **canonical symbol** (`footman`, `ancient_ground_near`), which the grammar guarantees came from the live roster / fixed place vocabulary. Units print as the raw symbol (`10 footman`, not `10 Footmen`) — a display-name table would be a second source of truth for a name `DT_Cards` already owns. **Flagged as a nicety, not fixed.**

---

## 6. The short-circuit — and an honest note on its size

`TryShortCircuitClarification()` runs **only** in `Clarify`, and **only** against a shortfall the game itself computed — the one question whose answer the game already holds. It handles: **negative** (discard, `Cancelled`), **affirmative** (take the number the game just offered), **bare number** (accept if ≤ available, otherwise re-ask). **Every one of those paths costs zero model calls.**

The classifier `SiegeAssistantParseClarificationReply` is a **pure free function** — no `UObject`, no `UWorld`, no snapshot, no model — the same shape `ValidateCommandAgainstSnapshot` and `USiegeAssistantGrammar::Build` were pinned into, and for the same reason: it is testable in isolation.

- **It is intentionally timid.** Closed word lists, **exact match only** after trim/lowercase/trailing-punctuation strip — never substring or prefix. "no archers" must not match "no"; that reply is a real order and belongs to the model. Anything not confidently classified returns false and costs one honest model call.
- **Bare integers only**, range read **from the grammar** (`GrammarCountMin`/`Max`), with a **length guard before `Atoi64`** so a 25-digit reply cannot overflow into an in-range value.
- **Multi-kind re-run:** after one shortfall is settled, `FindShortfall` re-runs on the resolved command, because a selection can be short in more than one place ("send 10 footmen and 5 sorcerers" with 8 and 2 available is two questions). Walking off the first answer would hand the confirm step a count the player never agreed to. Still zero model calls.

⚠️ **DEVIATION, STATED PLAINLY: the spec said "~15-line". The classifier is ~45 lines and the FSM branch ~40 (excluding comments) — call it 3–5×.** The extra is the negative branch, the re-ask branch, the multi-kind re-run and the overflow guard. I judged each worth its lines and I am flagging the overrun rather than trimming a correctness case to hit a number; if QA disagrees, the multi-kind re-run is the largest single block and the one to argue about.

### `FindShortfall`'s two deliberate refusals to answer
- ⛔ **It declines the ZERO case.** A kind with `0` orderable is the **guard's** to refuse through the unsupported-ask outcome (§6). If **any** kind in the selection is 0-orderable, `FindShortfall` returns false and lets 443's guard rule on the whole command. Two surfaces for one situation is how a player gets told two different things about the same order.
- ⚠️ **It only runs for the ZONE VERBS (Send/Guard/Ambush).** `GetOrderableCount` is `IsGroupCommandEligible`'s tally; **Follow's tally (`KindFollowable`) is not exposed by the snapshot** (TASK-441 exposed the orderable column only, per the §8 pin). Answering a follow shortfall from the wrong column would be a silent wrong answer, so **Follow gets no shortfall clarification in v1** — stated rather than left to be discovered.

---

## 7. §4/§5 — the snapshot, and the first-execution audit

- **ONE** `USiegeAssistantSnapshot`, created once in `BeginPlay`, re-`Capture()`d **per sentence**. `Capture` resets itself first, so no stale row survives.
- ⛔ **No unit registry, no actor cache, no dirty flag, no subscription list** — §4 rejects all four on sight and the clause is cited in the header.
- **The component never ticks** (`bCanEverTick = false`, `bStartWithTickEnabled = false`). The cheapest way to guarantee "never per tick" is to have no tick to put it in.
- **The team is resolved, never guessed.** No `ASiegePlayerState` ⇒ the turn is refused. A defaulted team would survey the wrong army and every downstream answer would be confidently wrong.

**§7 FIRST-EXECUTION AUDIT — what TASK-447 should grep for.** `ReportFirstCapture()` emits **once per session, at `Log` on `LogSiegeAssistant`**, on the first composed prompt:

- `zoneA_chars`, `zoneB_chars`, `zoneC_chars`, `zoneB+C_chars`, `MaxSnapshotChars`, `headroom`
- `roster_kinds` (distinct, i.e. `GetUnitKinds().Num()`), `MaxRosterKinds`, `roster_rows`, `places`
- **Zone B and Zone C pasted in full**
- the reference figures to compare against, in the line itself: the TASK-413 **fixture** at zoneB=68 / zoneC=887 / B+C=955 (13-kind board) and the **shipped builder** *derived* at B+C=738. **This is the first MEASURED shipped-builder reading.**
- Search key: **`FIRST LIVE CAPTURE of USiegeAssistantSnapshot`**.

**Also newly executable:** `BeginPlay` logs `zoneA_chars` and the resolved vocabulary. And outside Shipping, on the **second** turn only, `GetCachedZoneA()` rebuilds Zone A and compares it byte-for-byte against the cache — turning §8's QA criterion *"calling BuildZoneA twice in one process returns byte-identical strings"* from a documented claim into an **executed** one, for the cost of one extra string build per session. Search key: **`Zone A byte-identity re-check`** / **`ZONE A IS NOT BYTE-IDENTICAL`**.

**Zone A caching honours the snapshot's caller contract exactly.** Built once and cached; the vocabulary is resolved once and **a failed resolve is remembered as a result** (`bVocabularyResolved` is set *before* the load). Retrying later could succeed mid-session and change Zone A, throwing away every cached KV prefix. A null that stays null honours "pass the same vocabulary object every turn" exactly as a loaded asset does.

---

## 8. §2/§7 — fault posture and authority

- `bAssistantFaulted` is a **session latch** set only by `MarkAssistantFaulted`. It logs **once** at Error, aborts anything in flight, drops the latch/preview/pending state, enters `Failed`, pushes a template, and broadcasts `OnAssistantAvailabilityChanged(false, reason)`. **It disables the console and nothing else** — no key, no card, no stance, no group.
- **Match start can never be blocked:** `BeginPlay` touches no model and this component holds **no reference into the `SiegeLlama` plugin at all** (no include, no `Build.cs` dependency — both are 443's). That makes "a missing GGUF never blocks match start" structural rather than careful.
- `Failed` (a turn) and `bAssistantFaulted` (the session) are **deliberately separate**. One unparseable answer must not cost the player their console for the rest of the match.
- **Authority:** `SubmitUtterance` refuses on `!GetOwner()->HasAuthority()` with the keys' approved wording. The assistant refuses **exactly where the keys do** and is never more capable than them.

---

## 9. Console-widget wiring — a forward, not a translation

TASK-444's widget had already landed when I wrote this, so I **matched its declared inbound signatures character-for-character** instead of inventing a shape it would have to adapt to:

| component (out) | → | widget (in) |
|---|---|---|
| `OnAssistantStateChanged(uint8, const FString&)` | → | `SetAssistantState(uint8, const FString&)` |
| `OnAssistantMessage(const FString&)` | → | `ShowTranscriptLine(const FString&)` |
| `OnAssistantAvailabilityChanged(bool, const FString&)` | → | `SetConsoleEnabled(bool, const FString&)` |
| *[443 `EnterAwaitConfirm`]* | → | `ShowConfirmPrompt(const FString&)` |
| *[443 `ClearConfirmPreview`]* | → | `HideConfirmPrompt()` |

| widget (out) | → | component (in) |
|---|---|---|
| `OnConsoleSubmitted(const FString&)` | → | `SubmitUtterance(const FString&)` |
| `OnConsoleConfirmed()` | → | `ConfirmPressed()` |
| `OnConsoleCancelled()` | → | `CancelPressed()` |
| `OnConsoleOpenChanged(bool)` | → | `NotifyConsoleOpened()` / `NotifyConsoleClosed()` |

⚠️ **This file does not include `SiegeAssistantConsoleWidget.h` and holds no widget pointer**, deliberately: the FSM must not own a widget's lifetime, and a component that compiles against a `UUserWidget` cannot be driven by anything else (a console command, a test, a later voice path). The three delegates are the whole push surface.

⚠️ **Seed before you bind** (qa/TASK-005 major-2): push `GetStateAsByte()` + `GetStateLabel()`, `GetLastMessage()` and `IsConsoleAvailable()` into the widget **first**, then bind.

---

## 10. 🚩 FLAGGED — recorded, not fixed

1. **NO TASK IN THIS BATCH IS EXPLICITLY ASSIGNED THE WIDGET'S CREATION AND THE ABOVE BINDING.** TASK-440 makes the subobject, 442 (this) exposes the surface, 444 builds the widget, 443 owns the executor. **The mapping table above exists so whichever task takes it has nothing left to decide** — but somebody must take it, or the console will be built and never wired. **This is the single most likely way Wave 1 ships a console that does nothing.**
2. **Zone A never teaches the model what `pending:` means.** `BuildZoneA`'s schema/rules block names no `pending` key, yet Zone C always emits one and §1 prescribes it as the context carrier. This is the §9c seam class (few-shots and the rest of the prompt being one unenforced contract). I emit the pending line as §1 specifies and did **not** edit `SiegeAssistantSnapshot.cpp` (TASK-441's file this batch). **Recorded for a later prompt pass, which owes its own accuracy re-measurement.**
3. **A decline-without-a-command carries almost no context.** For `{"ask":"which_place"}` the model returned no partial command, so the pending line is only `problem: which place`. The next turn is a fresh single-turn call and the model will not remember asking. That is inherent to §1, not a defect I can fix here.
4. **Red-capture interaction, inherited from TASK-441's flag:** both eligibility predicates hardcode `Team == ETeamId::Blue`, so a Red capture reports an all-zero orderable column. Under my `FindShortfall` that means **every** kind hits the zero case, `FindShortfall` declines, and 443's guard refuses the order. **Safe (nothing executes) and confusing (everything refuses).** V1 is host/standalone Blue so it should not arise; the fix belongs in the shipped predicates at M8 P2, never here.
5. **No automation tests in this task.** The spec named exactly two files and asked for none; `Tests/` would be a third. The pieces worth testing are pure and ready for a follow-up: `SiegeAssistantParseClarificationReply` (free, pure), `SiegeAssistantReasonTemplate` / `SiegeAssistantStateLabel` (free, pure — a walk over every enum member would catch a missing row), and `DescribeCommandForPlayer` / `BuildPendingLine` (state-only, no world). **Recommending a follow-up task rather than silently widening this one.**
6. **The short-circuit's word lists are English-only.** So is the prompt, by design. Recorded so a later localization pass knows this table exists.
7. **`ShortfallCount`'s message says "{Available} can take that order"**, not "you have {Available}" — because the number is the **orderable** tally, not the roster total. A Cleric standing next to the footmen is not "unavailable", it is not zone-orderable. The wording was chosen to be true of the number actually used.

---

## 11. ⚠️ NOT VERIFIABLE WITHOUT A COMPILE — stated plainly

**I did not compile, and the module has one gate (TASK-447).** Everything below is reasoned from source, not observed:

- **It cannot compile alone and is not expected to** — it links against §9 / "Settings screen…" §8 symbols owned by sibling tasks. That is the TASK-395/396 and TASK-416/417 precedent; **do not open a QA loop over it.**
- **UHT surface unverified:** the two `UENUM`s, the `USTRUCT`, the three `DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`, and every `UFUNCTION`/`UPROPERTY`. Specific risk I traded down but cannot close: `GetLastMessage()` and `GetStateLabel()` return `FString` **by value** rather than `const FString&`, because a Blueprint-exposed reference return is the kind of thing UHT rejects late.
- **Complete-type include law:** I added `GameFramework/Actor.h` (for `HasAuthority()`), `GameFramework/Controller.h` (`->PlayerState`), `GameFramework/PlayerState.h` (the `Cast` source), `Engine/World.h`, and the four Siegebound headers whose members I call. If one is still missing it will be a `C2027`, and per the CONVENTIONS distinguishing test that is an **include** problem, not the most-vexing-parse `C2228`.
- **Most-vexing-parse law honoured:** the one `TSoftObjectPtr` construction from a hoisted path constant is **brace-initialised**.
- **Shadowing law honoured** by inspection: no local, parameter or loop variable named `Owner`, `PlayerState`, `Instigator`, `Controller` or `Slot`. Locals are `OwningActor`, `OwningController`, `OrderingState`.
- **String/API assumptions:** `FString::LeftChop`, `TrimStartAndEndInline`, `FChar::IsDigit`, `FCString::Atoi64`, `FCString::Strcmp`, `FText::Format` with `FFormatNamedArguments`, ranged-for over `FString`. I deliberately replaced `LeftChopInline(1, EAllowShrinking::No)` and `FString::Equals(const TCHAR*, …)` with `LeftChop` and `Strcmp` to remove overload/enum-version questions I could not settle without a compiler.
- **Nothing on screen is claimed.** No PIE, no editor, no MCP. The FSM's visible behaviour closes on rendered pixels or Jonathan's eyes at TASK-448 (ruling 9), and the log lines in §7 close at TASK-447's audit.
- ⚠️ **`RouteParsedCommand` and `ExecutePendingCommand` are stubs, so today the FSM never reaches `AwaitConfirm`, `Executed` or `Deferred` at runtime.** `EnterAwaitConfirm` and `EnterDeferredIntent` are declared, defined and **not called by any TASK-442 code path** — 443 calls them. That is the intended half-built state and it is why 443 must land before any behavioural claim is made.

---

## 12. Compliance checklist

- ⛔ **No multi-turn loop, no conversation state, no model output fed back** — four structural mechanisms, §2 above.
- ⛔ **No executor, no selector, no confirm step, no deferred-intent body, no `Build.cs` edit** — all eight seams named, empty, refusing safely.
- ✅ **States exactly as specced**, pushed as `uint8`.
- ✅ **Reason codes + the player-facing template table**, and it is the only place player text exists.
- ✅ **The short-circuit** — zero model calls on the clarification path (size deviation flagged in §6).
- ✅ **Snapshot per sentence, never per tick**; no registry/cache/dirty-flag/subscription; **first-capture character count logged**.
- ✅ **Fault posture** — session latch, console only, match start never blocked, no plugin reference.
- ✅ **Authority** — refuses with the keys' approved wording, never more capable than the keys.
- ✅ **No controller / snapshot / grammar / `Content/` / Git / compile / editor / MCP / PIE.**
- ✅ **M8 declaration duty stated verbatim** (top of this note and in the header).
- ⛔ **Nothing here is reported as accuracy progress.** This task changes what the game *does*, not what the model *emits*; the eval scores emitted JSON.
