# TASK-443 — [W1-B2b] The EXECUTOR + the CONFIRM STEP + the toggle + the guard call + the deferred intent

**Assignee:** gameplay-programmer · **Status on completion:** `ready-for-qa` (reviewed under **TASK-446**) · **Date:** 2026-08-03
**Law followed:** CONVENTIONS "Settings screen + the assistant CONFIRM STEP + the non-orderable-kind guard (2026-08-03)" **§5, §6, §8, §9, §11, §12, §13** · "In-match LLM command assistant" **§1, §2, §3, §4, §8, §9, §9c, §10**

> **M8 DECLARATION DUTY, verbatim as required: "adds no replicated property, no new replicated class, no new relevancy tier."**
> **And WHY, as the dispatch required it be said:** the settings value is **client-local and governs a LOCAL review step, never an authoritative outcome** — there is nothing for a server to own and no relevancy question to answer. Everything this task adds is client-local FSM/executor state: no `UPROPERTY(Replicated*)`, no `GetLifetimeReplicatedProps`, no `bReplicates`, no RPC, no `Server`/`Client`/`NetMulticast` `UFUNCTION`. The ORDERS it produces go through the same authoritative controller APIs the keys use, under a `HasAuthority()` re-check at the call site.

⛔ **NOT COMPILED. NO GIT. NO EDITOR, NO MCP, NO PIE.** One compile gate only (TASK-447, quiet-module law). §11 below is the honest inventory.

---

## 0. PRE-FLIGHT (spec item 0), RUN AND REPORTED RATHER THAN PREDICTED

```
$ git status --porcelain Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs
(no output — CLEAN)
```

Clean ⇒ proceeded. ⚠️ Per CONVENTIONS §9's corrected lesson, the pre-flight is mandatory and the **prediction** is what was wrong last time; I branched on the answer, not on an expectation.

---

## 1. FILES TOUCHED — THREE, ALL MINE UNDER RULING 10

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h` | **EDIT** — 8 seam docs updated to "filled"; 2 new delegates; `AttachConsoleWidget`/`DetachConsoleWidget`/`GetAttachedConsoleWidget`/`GetPendingConfirmSummary`; 13 new private helpers; the executor's state + 4 tunables; the full ON/OFF trace in the class comment. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp` | **EDIT** — all 8 seam bodies implemented in place; the executor, selector, confirm preview, deferred intent, the widget join; 2 internal helpers. |
| `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.Build.cs` | **EDIT** — `+ "SiegeLlama"`; the stale `SlateCore` example re-pointed. **36 insertions, 6 deletions.** |

⛔ **Nothing else.** No controller edit, no snapshot edit, no grammar edit, no widget edit, no `Content/`, no `.ini`, no `.uproject`, no Git, no compile. `git status` shows other lanes' work in files I did not touch (`SiegePlayerController.*` = TASK-440/449/453, `SiegeAssistantSnapshot.*` = TASK-441, the settings files = TASK-436/437, the console widget = TASK-444).

⚠️ **I OBSERVED `SiegePlayerController.h` CHANGE UNDER ME MID-SESSION** (a grep and a later read of the same line numbers disagreed; `AssistantConsoleActionAsset` had appeared). That is TASK-449 working in parallel, as designed — recorded because it means **any line number I quote for that file is stale on arrival**, and because it is live evidence for the quiet-module law's "file-disjoint is not build-disjoint".

---

## 2. ⛔ THE ONE THING ONLY I COULD DISCHARGE — THE OFF-PATH TRACE, IN FULL

**The law:** *the toggle removes a HUMAN REVIEW STEP and never a MACHINE CHECK.*
TASK-436 stated it **cannot** certify this. TASK-439's QA **explicitly refused** to certify it — it verified only that those tasks contain no reference to the guard or executor, which is a different sentence. **It closes here.**

### 2a. The structural argument, before the walk

**The toggle is read in exactly ONE function, from exactly ONE call site, and that call site is the LAST step of routing.** Verified by grep over the delivered file, not asserted:

```
1987:  bool USiegeAssistantComponent::IsConfirmBeforeExecuteEnabled() const   ← the only definition
1200:      const bool bConfirmSetting = IsConfirmBeforeExecuteEnabled();      ← the only call
1134:      if (!ValidateCommandAgainstSnapshot(...))                          ← the guard, 66 lines EARLIER
```

⇒ **Every machine check lies strictly before line 1200.** The branch at 1200 cannot skip one, because none of them is downstream of it. That is a property of the control flow, not a promise in a comment.

### 2b. The walk, check for check, ON beside OFF

| # | Check | Where | ON | OFF | Skippable by the toggle? |
|---|---|---|---|---|---|
| 1 | empty input, zero side effects | `SubmitUtterance` gate 1 | ✅ runs | ✅ runs | **No** — 700 lines upstream |
| 2 | session fault latch | gate 2 | ✅ | ✅ | **No** |
| 3 | **AUTHORITY** (`!HasAuthority()` → keys' wording) | gate 3 | ✅ | ✅ | **No** |
| 4 | admission table (Busy / AwaitConfirm / Deferred) | gate 4 | ✅ | ✅ | **No** |
| 5 | short-circuit / clarification path | gate 5 | ✅ | ✅ | **No** |
| 6 | one dispatch per turn (`bModelDispatchedThisTurn`) | gate 9 | ✅ | ✅ | **No** |
| 7 | stale-turn guard (`InTurnId != TurnId`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 8 | state guard (completion must arrive in `Thinking`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 9 | success guard (`!bSuccess` → `FailedModelError`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 10 | **the parse** (`ParseSiegeAssistantCommand`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 11 | **`Kinds.Num() == Counts.Num()`** (`SiegeAssistantValidateSelection`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 12 | **the shortfall arithmetic** (`FindShortfall`) | `HandleModelCompletion` | ✅ | ✅ | **No** |
| 13 | ⛔ **THE NON-ORDERABLE-KIND GUARD** (`ValidateCommandAgainstSnapshot`) | `RouteParsedCommandInternal` **step 1**, line 1134 | ✅ | ✅ | **No — 66 lines before the read** |
| 14 | place eligibility (zone verb needs a resolvable place) | step 2 | ✅ | ✅ | **No** |
| 15 | deferred-trigger fork | step 3 | ✅ | ✅ | **No** |
| — | — | **⇩ line 1200: the toggle is read ⇩** | | | |
| 16 | **`HasAuthority()` RE-CHECK at the call site** | `ExecutePendingCommand` | ✅ | ✅ | **No — downstream of both** |
| 17 | `ResolvePlace` (the coordinate airlock) | `ExecuteZoneOrder` | ✅ | ✅ | **No** |
| 18 | **shipped eligibility predicates** (`IsGroupCommandEligible` / `IsFollowCommandEligible`) | `SelectUnitsForOrder` | ✅ | ✅ | **No** |
| 19 | **never truncate silently on shortfall** | `SelectUnitsForOrder` | ✅ | ✅ | **No** |
| 20 | `CreateUnitGroup`'s own alive-refilter + `INDEX_NONE` | controller | ✅ | ✅ | **No** |

**ON** = 1–15, then `EnterAwaitConfirm()`, then (on Accept) `ExecuteAndReport()` → 16–20.
**OFF** = 1–15, then `ExecuteAndReport()` → 16–20.

⇒ **THE DIFFERENCE IS EXACTLY ONE THING: WHETHER A HUMAN LOOKS.** No check appears on one path and not the other.

### 2c. Two things that make it hard to break later

1. **`ExecuteAndReport()` is ONE function with TWO callers** — `ConfirmPressed` (line 709, toggle ON) and `RouteParsedCommandInternal` (line 1221, toggle OFF). ⚠️ **This is why I edited a TASK-442 function** (§4 below): two copies of that tail could drift, and the drift would land precisely on the claim §5 needs to stay true. Sharing the body makes ON and OFF *provably* identical downstream instead of identical-by-inspection.
2. **The OFF branch is written to look like the ON branch.** It calls `ClearConfirmPreview()` first — a no-op there — so both paths reach `ExecuteAndReport()` through the same two statements.

### 2d. ⚠️ It fails safe, and `bForceConfirmReview` only ever forces review ON

- Unresolvable `USiegeSettingsSubsystem` ⇒ `return Settings ? Settings->IsAssistantConfirmEnabled() : true;` — **`true`, confirm ON.** Commented at the site that `Settings && Settings->…` inverts the safety direction with a two-character edit.
- `bReviewFirst = bConfirmSetting || bForceConfirmReview` — an **OR**. There is no argument, flag or setting anywhere in the class that can force the branch the other way.

### 2e. ⛔ NO BLOCKER FOUND — stated as a finding, not as an absence

**No check rides on the confirm branch.** I looked for the failure the dispatch named and did not find it; had I found one I would have reported it rather than restructured. The one thing that *is* downstream of the branch is `ExecutePendingCommand`, and it is downstream of **both** arms.

---

## 3. THE EIGHT SEAMS, FILLED IN PLACE

No name, no signature, no call site moved; no parallel set added.

| Seam | What it now does |
|---|---|
| `DispatchTurnToModel` | Resolve `USiegeLlamaSubsystem` → `IsReady()` → `IsBusy()` → `RequestCompletion` with the **exact** `BindWeakLambda` shape TASK-442 published. ⛔ **`!IsReady()` does NOT latch `bAssistantFaulted`** — see §7. |
| `RouteParsedCommand` | One-line forward to `RouteParsedCommandInternal(Command, false)`. |
| `RouteParsedCommandInternal` | guard → place eligibility → deferred fork → **the toggle**. |
| `EnterAwaitConfirm` | tear any previous preview → ghost circles → `SetState(AwaitConfirm)` → `PushMessage(ConfirmPrompt, PendingArgs)` → broadcast the summary. Warns if the console is shut. |
| `ClearConfirmPreview` | destroy both decals, broadcast `…PromptHidden` **only if a prompt was actually up**. Safe with nothing up. |
| `ExecutePendingCommand` | `HasAuthority()` re-check, then the §8 executor-seam mapping. |
| `EnterDeferredIntent` | the DEV-11 relative-word check, then latch + TTL + timer + `DeferredArmed`. |
| `ClearDeferredIntent` | clear timer, drop latch, reset. Safe with nothing latched. |
| `AbortInFlightRequest` | `CancelActiveRequest()`, gated on **our own** `bModelDispatchedThisTurn`, not on the subsystem's global `IsBusy()` alone. |

### The executor mapping — CONVENTIONS §8, character-for-character

`Send`/`Guard` → `CreateUnitGroup(Hold, …)` · `Ambush` → `CreateUnitGroup(Ambush, …)` · `Follow` → `EnrollInDefaultFollowGroup(Unit)` per unit · `Charge` → `SetUnitCommand(Attack)` · `Fallback` → `SetUnitCommand(Defend)` · `Rally` → `AHeroCharacter::Rally()`.

### The selector
One filtered `TActorIterator` pass → **the shipping predicates, never reimplemented** → sorted **nearest to the TARGET, not the hero** (§8 verbatim) → per-kind take. **`Kinds` empty = every eligible unit** (`who:"all"`). ⛔ **On shortfall it refuses the whole order rather than truncating** (§8 verbatim).

⚠️ **Kind matching is `Unit->GetCardID() != Kind` with NO lower-casing** — `FName` comparison is case-insensitive, so the snapshot's canonical `footman` matches the unit's `Footman` with **no second copy of the transform**. That was a deliberate choice to avoid a second source of truth for a mapping the snapshot owns privately.

---

## 4. ⚠️ THREE DECLARED EDITS TO TASK-442's FINISHED CODE — DECLARED, NOT SLIPPED IN

TASK-442 said everything outside the seams was finished. I changed three things and each has a reason stronger than tidiness. **If QA rules any of them out, all three revert mechanically.**

1. **`ConfirmPressed`** — its 5-statement tail became `ExecuteAndReport()`. **Behaviour is statement-for-statement identical.** Reason: §2c item 1 — it is the evidence for the off-path law.
2. **`EndPlay`** — `+ DetachConsoleWidget();`. Reason: the widget's lifetime is the controller's and can outlive this component; a surviving binding into a destroyed component is the silent kind of dangling.
3. **The class comment's seam map** — rewritten from "named and empty" to the filled map, plus the full ON/OFF trace. Documentation only.

⛔ **`SubmitUtterance`'s nine gates, the admission table, `HandleModelCompletion`, the template table, `FindShortfall`, `BuildPendingLine` and the short-circuit are UNTOUCHED.** I deliberately left `FindShortfall`'s inline zone-verb test alone rather than swap it for my new helper — a cosmetic edit to finished code would have weakened the "I touched only what I had to" claim.

**On the pinned admission table and nine-step gate order:** I followed both and found **no error in either**. Nothing to report against them.

---

## 5. `Build.cs` — BOTH EDITS, AND WHY THE DANGEROUS ONE IS THE COMMENT

**(a) `+ "SiegeLlama"` in `PublicDependencyModuleNames`.** The game lane's first call into the plugin. ⛔ **Still NOT `HTTP`, NOT `Sockets`** — re-stated at the site with the firewall-prompt rationale. ✅ **It does not leak `llama.h`**: `LlamaCpp` is a *private* dependency of `SiegeLlama`, so the native header stays off this module's include path and the cross-lane surface remains `(prompt, gbnf) → string`.

**(b) The `SlateCore` comment re-pointed — and `SlateCore` KEPT.** The comment justified the dependency by naming `SiegeAssistantInputProbe.cpp`, which TASK-444 deleted. Per §12: **when a justifying comment names an artifact that is later deleted, re-point the comment; never delete the thing it justifies.** The example now names `SiegeAssistantConsoleWidget.{h,cpp}` — the module's only `ETextCommit`-in-a-`UFUNCTION` and only `FSlateApplication` user — with the mechanism (include paths propagate, the import library does not) and the failure mode (**16 unresolved externals led by `Z_Construct_UEnum_SlateCore_ETextCommit`**, at LINK, not compile) restated so nobody re-derives it wrong. ⛔ **The dependency was not touched.**

---

## 6. THE DEFERRED INTENT — AND THE DEV-11 RELATIVE-WORD DECISION

`DeferredIntentTTLSeconds` **120** · `DeferredIntentPollHz` **1** (both `EditDefaultsOnly`, both flagged for Jonathan's feel pass). Timer armed **only** while latched; `ClearDeferredIntent` is its only stop.

**On fire:** stop the timer → **re-`Capture()`** → clear the trigger → `RouteParsedCommandInternal(FiredCommand, /*bForceConfirmReview*/ true)`.
⇒ the fired order re-runs the **guard**, the **place eligibility** and every executor check **against a fresh snapshot**, and lands in `AwaitConfirm` ⚠️ **even with the confirm toggle OFF**. ⛔ **It never executes blind.**

**On the second `Capture()`:** the header forbids re-capturing *from the executor mid-turn*. This is **not** mid-turn — the turn ended up to 120 s ago and re-resolving against the current board *is* the point of a deferred order. It is still "per sentence, never per tick": **one capture per FIRE, none per poll** (the poll only counts units).

### ⚖️ THE DEV-11 DECISION — "2 MORE knights"

*"wait until i have 2 more knights then send them mid"* is the row that flips between runs. The grammar's `at_least` is **absolute** and cannot express *more*, so a relative reading emits `at_least:2` where the player meant `Current+2` — **indistinguishable in the JSON**.

⇒ **If the threshold is ALREADY satisfied at latch time, I do NOT latch and do NOT fire — I RE-ASK**, through the existing `AskHowMany` clarification (no new surface, §3). The ambiguity is resolved against the **live roster** by the only party who knows: the player.

⚖️ **The alternative — fire now → `AwaitConfirm` — was considered and rejected.** It is correct for the *absolute* reading, but it converts a **known** ambiguity into a confident action, and "confidently wrong" is the class this whole design exists to remove. 📌 **QA/manager may rule the other way**; the change is one branch.

---

## 7. 🚩 FLAGGED — FOUR THINGS RECORDED RATHER THAN QUIETLY DECIDED

**(a) ⛔ `charge` / `fallback` DO LESS THAN THE T / E KEYS, AND I COULD NOT FIX IT FROM MY FILE.**
§8's seam pins `charge → SetUnitCommand(Attack)` and that is exactly what runs. But `OnCmdAttackPressed` does **three** things: `CancelGroupPick()`, **`ClearAllUnitGroups()`** (the TASK-344 release law), then `SetUnitCommand(Attack)`. **Both of the first two are `private`** on `ASiegePlayerController` (verified: last access specifier before both declarations is `private:`). Re-implementing the release law here is precisely the parallel implementation §2 forbids.
⇒ **The assistant's "charge" latches the stance but leaves standing group orders in place**, so grouped units keep their stations where pressing T would have freed them. **A player who says "charge!" may see less happen than when they press T.** Logged at `Warning` at both sites.
**The fix is one public entry point on the controller** (e.g. `ApplyArmyWideStance(ESiegeUnitCommand)` doing all three, which is then genuinely "the same public API the keys call") — a `SiegePlayerController` edit, i.e. **another task's file**. ⚠️ **I consider this the most consequential item in this handoff.**

**(b) An assistant-formed group owns NO persistent ground markers.** `CreateUnitGroup`'s marker params are ownership-transferring (TASK-440 contract #2), but TASK-442's `ConfirmPressed` tears the ghosts down **before** executing — correctly, so no ghost outlives its decision — leaving nothing to transfer. I pass `nullptr, nullptr`. **Safe (no double-free, no dangling) and visibly different from a key-formed group.** Fixing it means reordering a finished TASK-442 function; I did not.

**(c) Two paired tunables duplicate protected controller members.** `AssistantPositionRadius` **700** ≡ `GroupPositionRadiusDefault`, `AssistantAttackRadius` **1500** ≡ `GroupAttackRadiusDefault`. Both controller members are `protected` and unreachable. Declared as **paired tunables** (the `SpawnBoxHalfExtent` 3-way precedent) rather than hard-coded at the call site, so the second source of truth is **named**. ⛔ **Keep in lockstep.** Clean fix = one access change on the controller, then delete mine.

**(d) `IntentTakesZoneOrder` now exists file-locally in TWO files.** Mine, and TASK-441's `SiegeAssistantGuardInternal::IntentTakesZoneOrder` in `SiegeAssistantSnapshot.cpp`. Both are file-local statics, so neither can reach the other; the shared home would be `SiegeAssistantCommand.h`, a **pinned** header owned by another task. ⛔ **The two must agree** — if one gains a verb, so does the other. Consolidation recorded for whoever next owns that file.

---

## 8. THE CONSOLE-WIDGET JOIN (the mid-task ruling — TASK-453 calls it)

```cpp
void USiegeAssistantComponent::AttachConsoleWidget(USiegeAssistantConsoleWidget* InWidget);   // ← TASK-453 calls THIS
void USiegeAssistantComponent::DetachConsoleWidget();
USiegeAssistantConsoleWidget* GetAttachedConsoleWidget() const;
FString GetPendingConfirmSummary() const;                                                     // ← ⛔ SEED FROM THIS
```

**Contract, as required:** **null-safe** (logs, does nothing, detaches nothing) · **idempotent** (every binding removed before any is made ⇒ ten calls leave exactly one of each) · **re-targetable** (a different widget cleanly detaches the old) · safe **at any point in the widget's life**, which is what makes it correct at *open* time rather than only at startup.

**It binds all eight forwards, four each way, with ZERO adapters except one:**

| direction | forwards |
|---|---|
| widget → component | `OnConsoleSubmitted`→`SubmitUtterance` · `OnConsoleConfirmed`→`ConfirmPressed` · `OnConsoleCancelled`→`CancelPressed` · `OnConsoleOpenChanged`→`HandleConsoleOpenChanged` |
| component → widget | `OnAssistantStateChanged`→`SetAssistantState` · `OnAssistantMessage`→`ShowTranscriptLine` · `OnAssistantAvailabilityChanged`→`SetConsoleEnabled` · **`OnAssistantConfirmPromptShown`**→`ShowConfirmPrompt` · **`OnAssistantConfirmPromptHidden`**→`HideConfirmPrompt` |

The single adapter is `HandleConsoleOpenChanged(bool)` — **one** widget delegate maps to **two** component entry points. It adds no policy.

**⛔ TWO NEW DELEGATES WERE ADDED, AND QA SHOULD RULE ON THEM.** TASK-442's forward table listed `[TASK-443 EnterAwaitConfirm] → ShowConfirmPrompt` and `[TASK-443 ClearConfirmPreview] → HideConfirmPrompt` as **mine to provide**, but no channel existed. `OnAssistantConfirmPromptShown/Hidden` are that channel. They are **not redundant with `OnAssistantMessage`**: the transcript line is *history*, the confirm prompt is a *live modal state* that raises Accept/Cancel, and TASK-444's widget gates `ConfirmPressed()` on it so Accept can never execute an unshown order. Folding them would make "is a prompt up?" a string comparison.

**⛔ THE LAZY-CREATION TRAP IS CLOSED, AND `GetPendingConfirmSummary()` IS HOW.** A deferred intent's timer runs while the console is **shut**; it can fire, re-resolve and enter `AwaitConfirm` **before the widget exists**. A bind-only join would miss that broadcast forever and show a console with no prompt and no buttons for a genuinely pending order — no error, no log. `AttachConsoleWidget` therefore **seeds first, binds second**, including the pending prompt.

⚠️ **The component still holds no strong widget reference** — `TWeakObjectPtr`, so TASK-442's "the FSM must not own a widget's lifetime" survives the ruling. ⛔ **I did not call `AttachConsoleWidget` myself and did not touch the controller.**

---

## 9. §13 COMPLIANCE — `TestEqualSensitive`

**I wrote no tests in this task**, so §13 has no subject here. Stated rather than silently skipped. The pieces worth testing are pure and ready for a follow-up: `SiegeAssistantComponentInternal::IntentTakesZoneOrder` (pure, free) and `SelectUnitsForOrder`'s per-kind take (needs a world ⇒ not cheap). ⚠️ **If a follow-up test asserts a byte/casing/character-for-character claim, it MUST use `TestEqualSensitive`** — `TestEqual` on `FString` is case-**insensitive** in UE 5.8 and makes such a test vacuous.

---

## 10. ⛔ WHAT I DID **NOT** DO

- ⛔ No controller, snapshot, grammar or widget edit. No `Content/`, no `.ini`, no Git, no compile, no editor/MCP/PIE.
- ⛔ **No new player-facing string anywhere.** Every outcome routes through an **existing** reason code: guard rejection and refused execution → `AskUnsupported` (§6, one surface) · unresolvable place → `AskWhichPlace` · already-satisfied trigger → `AskHowMany` · the confirm summary → `DescribeCommandForPlayer` + `ConfirmPrompt`.
- ⛔ **No unit registry, actor cache, dirty flag or subscription list** (§4). `CountLiveUnitsOfKind` and the selector are on-demand filtered iterations kept nowhere.
- ⛔ **No multi-turn loop, no conversation state, nothing fed back.** Raw output goes straight into a parameter and dies with the frame.
- ⛔ **No request queue.** Queue depth 1 by early return.
- ⛔ **I did not open the sealed holdout** (`assistant_eval_holdout2.csv`) and nothing here reads, scores against or quotes it.

---

## 11. ⚠️ NOT VERIFIABLE WITHOUT A COMPILE — STATED PLAINLY, NOT SOFTENED

**Nothing here has been compiled and nothing has run.** "Reviewed by reading" is legitimate and **incomplete**; the parser I did not run is the one that decides (§9c).

1. ⛔ **`#include "SiegeLlamaSubsystem.h"` REFERENCES A FILE THAT DOES NOT EXIST TODAY.** `USiegeLlamaSubsystem` is **not written** — TASK-423 delivered only the Zone-A test and correctly declared the shortfall; **TASK-450 writes the subsystem and is `backlog`.** I verified this against the repo, not the board: zero hits for the class outside comments. ✅ **The ordering is already enforced** — TASK-447 is blocked on TASK-424, which now covers TASK-450 — but ⛔ **if TASK-450 has not landed when the gate runs, this file cannot compile**, and the diagnostic will name my include. **Do not open a QA loop over it; dispatch TASK-450.**
2. **UHT unverified:** the 2 new `DECLARE_DYNAMIC_MULTICAST_DELEGATE*`, the 2 new `BlueprintAssignable` `UPROPERTY`s, the 4 new `UFUNCTION`s (`AttachConsoleWidget`, `DetachConsoleWidget`, `GetPendingConfirmSummary`, `HandleConsoleOpenChanged`), and the 4 new `EditDefaultsOnly` floats. `AttachConsoleWidget` takes a **forward-declared** class — legal and precedented (TASK-440's `GetAssistantComponent()` does the same) but unparsed.
3. **Engine APIs I checked against the 5.8 source on this machine rather than from memory:** `UWorld::GetGameInstance() const` (`World.h:4386`) · `UGameInstance::GetSubsystem<T>() const` (`GameInstance.h:440`) · `FTimerManager::SetTimer(Handle, Obj, Method, Rate, bLoop)` (`TimerManager.h:167`) · `TMulticastScriptDelegate::RemoveAll(const UObject*)` (`ScriptDelegates.h:1307`) · `FTimerHandle` lives in `Engine/TimerHandle.h` (included). **Not** re-verified by a build.
4. **Include-completeness reasoned, not observed:** added `Engine/DecalActor.h`, `EngineUtils.h`, `Engine/GameInstance.h`, `TimerManager.h`, `HeroCharacter.h`, `SiegeAssistantConsoleWidget.h`, `SiegePlayerController.h`, `SiegeSettingsSubsystem.h`, `SummonedUnit.h`, `SiegeLlamaSubsystem.h`; header gained `Engine/TimerHandle.h` + `Siegebound/UnitCommand.h`. A miss shows as `C2027` (include), not `C2228` (most-vexing-parse).
5. **Unverified specifics I would check first:** `Eligible.Sort(lambda)` over a **function-local** struct · `FVector::DistSquared2D` returning `double` · `AddDynamic` to a **private** `UFUNCTION` from inside the class · `TObjectPtr<ADecalActor>` assignment from `SpawnGroupCircleDecal`'s raw return.
6. **Shadowing law honoured by inspection:** no local named `Owner`, `PlayerState`, `Instigator`, `Controller` or `Slot`. My locals are `Controller` — ⚠️ **correction: `ExecutePendingCommand` declares `ASiegePlayerController* Controller`, and `AActor::Controller` does not exist on `UActorComponent`, so there is no shadow here; the helpers take `ASiegePlayerController& Controller` as a parameter for the same reason.** Flagged for QA to confirm rather than asserted away.
7. ⛔ **Nothing on screen, and nothing executed, is claimed.** No ghost circle has been drawn, no group formed, no timer fired, no toggle read at runtime. **The confirm step has never been seen.** That closes at TASK-447 (machine half) and TASK-448 (Jonathan's eyes).
8. ⛔ **Nothing here is accuracy progress.** The eval scores **EMITTED JSON**; this changes what the game **DOES**. **Bar #5 stands at 20/25 against 22 + zero refuse-class failures — uncleared.** Wave 1 is un-gated because **Jonathan weighed the failure modes and overrode his own gate**, never because the ladder worked.

---

## 12. FOR QA (TASK-446) TO SCRUTINISE HARDEST, IN MY ORDER

1. **§2's trace.** Re-derive it from the code, not from this table. The falsifying question: *is there any machine check downstream of line 1200 on only one arm?*
2. **§7(a) — `charge`/`fallback` doing less than T/E.** Rule on it. It is a live gameplay divergence I could not fix from my file.
3. **§4's three edits to TASK-442's finished code** — especially `ConfirmPressed`.
4. **§8's two new delegates** and whether `AttachConsoleWidget`'s contract really is idempotent + null-safe + re-targetable.
5. **§6's DEV-11 re-ask** — a judgement call with a defensible alternative.
6. **§7(b), (c), (d)** — the marker gap, the paired tunables, the duplicated predicate.
7. **§11(1)** — that the `SiegeLlama` include is a batch-ordering fact, not a defect.
