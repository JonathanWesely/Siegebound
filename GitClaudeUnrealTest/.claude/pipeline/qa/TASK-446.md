# QA Report — TASK-446 (W1-QA2, the game-lane gate) + TASK-457 plugin appendix

**Date:** 2026-08-03 · **Reviewer:** qa-reviewer · **Method:** source review only.

## Verdict: **PASS** — 0 BLOCKERS · 9 WARN · 6 NIT

| task | verdict |
|---|---|
| TASK-440 | ✅ PASS |
| TASK-441 | ✅ PASS |
| TASK-443 | ✅ PASS |
| TASK-444 | ✅ PASS |
| TASK-449 | ✅ PASS |
| TASK-451 | ✅ PASS (WARN-1 attached) |
| TASK-453 | ✅ PASS |
| TASK-454 | ✅ PASS |
| TASK-455 | ✅ PASS |
| TASK-456 | ✅ PASS (WARN-2, WARN-4 attached) |
| **TASK-457** (plugin appendix, cross-lane) | ✅ PASS (WARN-3 attached) |

---

## 0. ⛔ WHAT I DID NOT RUN — SAID FIRST, NOT AS A FOOTNOTE

**Nothing in this batch has been compiled and nothing has executed.** I ran no compiler, no UHT, no
linker, no automation runner, no editor, no PIE, no MCP.

- ⛔ **I do not say "the tests pass." No test in this repo has ever executed, before or after
  TASK-451's sweep.** Every statement below about a test is a statement about **source**. §9c's law
  binds: where an artifact has a parser, **the parser is the reviewer of record**, and here the
  parsers I did not run are **UBT/UHT** and **the automation runner**. Discharge is TASK-447's compile
  **plus a green run**, claimed against that run and nothing else. ⛔ **A converted assertion is not a
  passing assertion.**
- ⛔ **WARN-5 (`qa/TASK-424.md`) remains INSTRUMENTED, NOT DISCHARGED.** Nothing in this gate moves it.
- **Tooling discipline:** every comment-level and structural finding below was read from **raw `Read`
  output**. I reproduced §14 instance 1 live — `Grep` rendered `SiegeAssistantConsoleWidget.h:23` as
  `\** Cancel was pressed…`; raw `Read` shows `/** Cancel was pressed…`. It also rendered
  `SiegeLlamaSubsystem.cpp:1478` as `\ GRANULARITY`; raw `Read` shows `// GRANULARITY`. **Neither is a
  finding.** `Grep` located; `Read` decided.
- **Offsets:** every `file:line` below was re-verified **at the current file**, by symbol first
  (§18c). Several numbers in this gate's own criteria had already rotted and I did **not** file them —
  see NIT-4.

---

## 1. ⛔ CRITERION (a) + (k) — THE TOGGLE'S OFF-PATH. **CLOSED HERE, BEHAVIOURALLY, AT THE CODE.**

This is the debt TASK-439 explicitly refused to certify (it established only the **scope fact** that
TASK-436/437 reference no assistant code). **That is not the sentence "every check still runs with the
toggle off," and I am now asserting the behavioural sentence on my own trace.**

**The structure, re-derived from the file, not from 443's table:**

| fact | verified at |
|---|---|
| `IsConfirmBeforeExecuteEnabled()` — the **only** definition | `SiegeAssistantComponent.cpp:2079` |
| its **only** call site, the **last** step of `RouteParsedCommandInternal` | `:1293` |
| the non-orderable guard `ValidateCommandAgainstSnapshot(...)` — routing **step 1** | `:1227` |
| place eligibility — step 2 | `:1256-1273` |
| deferred fork — step 3 | `:1284-1288` |
| `ExecuteAndReport()` — **exactly two callers**: `ConfirmPressed` and the OFF fall-through | `:768` · `:1314` |
| `ExecutePendingCommand()` — **exactly ONE caller**, `ExecuteAndReport` | `:1324` |

⇒ **Every machine check is strictly upstream of the branch, or strictly downstream of BOTH arms.**
The ON arm is `EnterAwaitConfirm()` (`:1305`) → `ConfirmPressed` → `ExecuteAndReport()`; the OFF arm is
`ClearConfirmPreview(); ExecuteAndReport();` (`:1313-1314`). **The single difference is whether a human
looks.** That is the strong, structural form, not an inspection.

- ✅ **Fail-safe confirmed:** `return Settings ? Settings->IsAssistantConfirmEnabled() : true;`
  (`:2091`) — an unresolvable subsystem behaves as **ON**.
- ✅ **`bReviewFirst = bConfirmSetting || bForceConfirmReview`** (`:1294`) is an **OR**: the deferred
  fire can only ever force review **ON**. `RouteParsedCommand` forwards `false` (`:1196`); the deferred
  fire is the only `true` (`:1970`).
- ✅ **Downstream checks are on both arms:** `HasAuthority()` re-check (`:1469`), `ResolvePlace`
  airlock (`ExecuteZoneOrder:1530`), shipped eligibility predicates + **refuse-never-truncate on
  shortfall** (`SelectUnitsForOrder:1762`, `:1771-1786`).
- ✅ **Non-drifting, not merely true today.** `ExecuteAndReport` is **one body with two callers**, so
  ON and OFF cannot diverge under a later edit. **TASK-456's edit is the first such later edit and the
  structure held** — it landed inside `ExecutePendingCommand`, which has one caller and is downstream
  of both arms (verified independently at `:1324`, not taken from 456's handoff).

⇒ **CRITERION (a) MET. CRITERION (k) MET — verified as structure, not as conclusion.** ✅ And 443
reported it as a **finding** rather than as an absence of findings, which is the distinction this gate
was told to police.

## 2. ⛔ CRITERION (j) — THE LAZY-CREATION TRAP. **NOT PRESENT. NO `BeginPlay` BINDER EXISTS.**

Verified by **relative order at the symbol**, which is the durable assertion:

```
SetAssistantConsoleOpen(true)        SiegePlayerController.cpp:4298   ← guard, refusal returns :4302
GetOrCreateAssistantConsoleWidget()  :4305                            ← creation, unreachable on refusal
Assistant->AttachConsoleWidget()     :4348                            ← TASK-453
Console->OpenConsole()               :4357
```

- ✅ **Attach precedes open**, and the mechanism is confirmed at the widget, not taken on faith:
  `USiegeAssistantConsoleWidget::OpenConsole()` **ENDS** with `OnConsoleOpenStateChanged(true);` then
  `OnConsoleOpenChanged.Broadcast(true);` (`SiegeAssistantConsoleWidget.cpp:483-484`), and the
  component subscribes to that delegate **inside** `AttachConsoleWidget` (`:2175`). **Attaching after
  would miss the only "opened" broadcast of the first press.** ⇒ **i-3 RATIFIED.**
- ✅ **It is on the per-open path, so close-and-reopen is correct by construction**, not by the widget
  happening to persist. `BeginPlay` (`SiegeAssistantComponent.cpp:519-538`) contains **no binder** —
  it does `EnsureSnapshot()` and a Zone-A pre-warm only.
- ✅ **`AttachConsoleWidget` is genuinely idempotent, null-safe and re-targetable** — verified at the
  body (`:2127-2194`): null ⇒ log + return, **detaches nothing** (`:2129-2137`); `DetachConsoleWidget()`
  called **first** so every binding is dropped before any is made (`:2142`); `DetachConsoleWidget`
  removes by **object** (`RemoveAll`), both directions, all 8 forwards (`:2212-2221`).
- ✅ **SEED-THEN-BIND is real and it closes the deferred-fire hole.** `:2150-2167` pushes state,
  availability, last transcript line and **the pending confirm prompt** before any binding. Traced the
  live case: a deferred fire with the console shut reaches `EnterAwaitConfirm` (`:1334`), sets
  `bConfirmPromptUp = true` (`:1354`) and warns (`:1368`); a later open seeds it via
  `GetPendingConfirmSummary()` (`:2113-2121`), and `NotifyConsoleOpened` **preserves** any non-Idle
  state (`:575-578`). **The prompt survives.**
- ✅ **All four outbound widget delegates reach the FSM** and all five component delegates reach the
  widget, signatures matched pairwise (`FOnAssistantStateChanged(uint8, const FString&)` →
  `SetAssistantState(uint8, const FString&)`, etc.). The **one** adapter is
  `HandleConsoleOpenChanged(bool)` (`:2233-2246`) — arity only, `UFUNCTION()` at
  `SiegeAssistantComponent.h:1083`, no policy.
- ✅ **Single-owner held:** TASK-453 touched only `SiegePlayerController.cpp`; TASK-443 touched only
  the component + `Build.cs`. No second writer in either file.

⇒ **CRITERION (j) MET.**

## 3. CRITERION (f) — THE OPEN-KEY SEAM (TASK-449)

- ✅ **Guard first, UI second.** Statement 4 is `if (!SetAssistantConsoleOpen(true)) { return; }`
  (`:4298`); creation at `:4305` is **structurally unreachable** on refusal. On refusal: no widget, no
  viewport add, no focus move, one log line (`SetAssistantConsoleOpen`'s own, `:4225`).
- ✅ **Posture cannot stick true.** `OnConsoleOpenChanged` → `HandleAssistantConsoleOpenChanged`
  (`AddDynamic` at `:4434`), and `bOpen == false ⇒ SetAssistantConsoleOpen(false)` (`:4386`). A close by
  **any** route clears the posture. Re-entrancy terminates by construction: `CloseConsole` clears
  `bConsoleOpen` **before** re-broadcasting (`SiegeAssistantConsoleWidget.cpp:494` before `:517`).
- ✅ **Exactly ONE writer of `bAssistantConsoleOpen`** — `SiegePlayerController.cpp:4240`, inside
  `SetAssistantConsoleOpen`. `EndPlay` clears through the function (`:363`), never by assignment.
- ✅ **`ResolveInputAction` idiom + inert-null-safe.** Binding is guarded (`:482`); with the asset
  absent the action is null, the binding is skipped, the key is inert. No crash, no other action
  affected.
- ✅ **⛔ NO EXISTING `BindAction` RE-ROUTED.** The new binding is appended inside the existing
  `UEnhancedInputComponent` block; every shipped action is untouched. §2 holds.
- ✅ **Statement 8 is right and it matters:** `OpenConsole()` **can refuse silently and has no return
  value** (`SiegeAssistantConsoleWidget.cpp:453-461`, the disabled-console guard). 449 asks
  `IsConsoleOpen()` (`:4365`) and releases the posture. **Asking the widget instead of assuming the
  void call worked is the correct discipline.**

⇒ **CRITERION (f) MET.**

## 4. CRITERION (h) + §11 — THE CODE-AUTHORED TREES

| widget | tree order | child wiring |
|---|---|---|
| `USiegeAssistantConsoleWidget` | `ConstructConsoleTree(); return Super::RebuildWidget();` — **`.cpp:109-110`** ✅ | **`NativeConstruct()`** → `WireChildWidgets()` — `.cpp:402/418` ✅ |
| `USettingsMenuWidget` | `Initialize(); ConstructSettingsTree(); return Super::RebuildWidget();` — **`.cpp:92-94`** ✅ | `NativeConstruct` (`.cpp:318-323` documents the choice) ✅ |

- ✅ **Neither class overrides `NativeOnInitialized`** (verified repo-wide: the only override is
  `USessionMenuWidget.cpp:13`, which is **WBP-backed and correct there**). §11's asymmetry honoured.
- ✅ **Escape hatches present in both**: `BindWidgetOptional` on all six console children
  (`SiegeAssistantConsoleWidget.h:380-401`) and whole-tree skip on `WidgetTree->RootWidget != nullptr`
  (`.cpp:124` / `SettingsMenuWidget.cpp:114`), plus per-member null construction.
- ✅ **BIE params are FString/bool/uint8 only** — `SiegeAssistantConsoleWidget.h:318-331`. FSM state is
  a `uint8` and **the widget never interprets it**.
- ⇒ **i-2 RATIFIED. Neither widget is filed as a deviation — the LAW was the defective artifact.**

## 5. THE FOUR DECLARED DEPARTURES — **ALL FOUR RATIFIED, EACH ON ITS OWN MECHANISM**

| # | departure | mechanism I checked myself | ruling |
|---|---|---|---|
| **i-1** | `ToggleConsole()` never called | `SiegeAssistantConsoleWidget.cpp:520-530` — it calls `OpenConsole()` directly, i.e. it **shows the widget before the controller can gate it**, the exact ordering the ⛔ guard-first clause forbids. The controller performs the toggle itself. | ✅ **RATIFY** |
| **i-2** | Tree built before `Super::RebuildWidget()` | `UserWidget.cpp:1214` returns an `SSpacer` when `RootWidget` is null; both widgets ship the correct order. | ✅ **RATIFY** |
| **i-3** | Attach placed before `OpenConsole()` | `OpenConsole()` **ends** with `OnConsoleOpenChanged.Broadcast(true)` (`:484`); the component subscribes **inside** `AttachConsoleWidget` (`:2175`). | ✅ **RATIFY** |
| **i-4** | 454 refused to fold the release into `SetUnitCommand` | `SetUnitCommand` is a **pure latch** (`SiegePlayerController.cpp:976-1004`: authority check → latch → broadcast, nothing else) · `ClearAllUnitGroups`' doc reads *"Called by T/E (before SetUnitCommand)"* and folding makes it self-referential · **`ServerSetUnitCommand` is already named against the pure-latch contract at `:981`**. | ✅ **RATIFY** |

✅ **AND I RATIFY *HOW* 454 CLEARED THE SAFETY QUESTION — the positive control is the required
pattern.** Before trusting a zero-hit search for Blueprint callers of `SetUnitCommand`, it proved the
probe *could* find one: `OnUnitCommandChanged` **does** hit `Content/UI/WBP_HUD.uasset`. That converts a
negative from a §14 blind spot into **evidence**. ⛔ **Every future "nothing references X" claim in this
pipeline must follow it.**

✅ **ALSO RATIFY the `HasAuthority()` gate 454 DECLINED to add.** Verified the shipped posture: a
client's T runs the pick abort and the group release locally and is refused **inside**
`SetUnitCommand` (`:982-988`). An outer gate would have silently changed **shipped key behaviour**
under cover of a consistency fix. Declining it is the restraint §2 asks for.

## 6. TASK-455's THREE REFUSALS + THE DECLARED RACE — **ALL RATIFIED**

- **(i-5a) The trimmer role.** ✅ Verified `SnapshotTrimBudgetChars = 1085` at
  `SiegeAssistantSnapshot.h:298`, and the **only** real code reader is the roster trimmer:
  `const int32 RosterBudget = SnapshotTrimBudgetChars - ZoneBCharReserve - Head.Len() - Tail.Len();`
  (`SiegeAssistantSnapshot.cpp:1412`). ⛔ **The trimmer reads NEITHER plugin constant — confirmed.**
  Re-pointing it at the pre-filter's 3000 would have converted graceful narrowing into a **refused
  turn**. ✅ **RATIFY.**
- **(t) The count.** ✅ **0 `MaxSnapshotChars` code references remain — verified myself.** A repo-wide
  search over `Source/` returns **4 lines, all inside `/** */` doc comments**:
  `SiegeAssistantSnapshot.h:206`, `:208`, `:210` (the retirement notice) and `:352` (the dated
  parenthetical in `MaxUtteranceBytes`' comment). **Zero code, zero log strings.**
- **(i-5b) `static_assert`s instead of duplicates.** ✅ All three verified at
  `SiegeAssistantComponent.cpp:52`, `:59`, `:71`; each **binds the real symbols**
  (`USiegeLlamaSubsystem::SnapshotPreFilterMaxChars` and the snapshot's own), and the plugin constants
  exist (`SiegeLlamaSubsystem.h:178` `MaxSnapshotTokens = 400`, `:199` `SnapshotPreFilterMaxChars = 3000`).
  ⛔ **NO game-lane duplicate was added — confirmed by search: every game-lane occurrence of those two
  names is a comment, a log string or a qualified read.** All three hold on today's values
  (1085 < 3000 · 192 < 1085 · 480 < 893). ✅ **RATIFY — strictly stronger than the spec asked.**
- **(i-5c) `ZoneBCharReserve` stays 192.** ✅ **RATIFY.** The instrument (`ReportFirstCapture`,
  `.cpp:2720`) is correct and **has never executed**; the 68/71 figures on record came from the
  **spike's** `AppendZoneB`, a different lane. The spec's own fallback clause fired. **A derived number
  substituted here would have been the precise defect §12g exists to prevent.** It made the reading
  **takeable** (`:2738-2748` prints the reserve, the live `zoneB_chars` and the over-charge). Reading
  pinned on TASK-447.
- **(s) The declared race.** ✅ **RATIFY.** `EnsureStaticPrefixRegistered` is called from
  `NotifyConsoleOpened` (`:596`) and, as backstop, from `DispatchTurnToModel` (`:1161`) — **after** the
  `IsBusy()` guard, **before** `RequestCompletion`. The comment at `:1141-1149` writes the window in as
  a **race**, not an ordering guarantee, and states the benign worst case. **Confirmed against the
  plugin:** `Run()` calls `ApplyPendingStaticPrefix()` (`SiegeLlamaSubsystem.cpp:626`) **before** the
  `bWorkPending` branch (`:628`). ⚖️ **An agent that declines to over-claim about its own work is the
  behaviour this wave has been asking of reviewers.**
- **(i-5b, `IsReady()` gate) — the highest-consequence line, and it is right.**
  `SiegeAssistantComponent.cpp:2772-2775` returns silently and **unlatched** when the model is not
  ready. Confirmed the mechanism it protects against: an eager call would have latched, logged success,
  and left both budgets inert for the session. **This is the correct direction.**

## 7. **(r) THE `SetStaticPrefix` COUPLING — RULED ON THE MERITS, NOT ON THE CONSEQUENCE**

I note first that the consequence exists (a ruling against it leaves §8's mandatory assertion with no
implementable form), and I record it as **explicitly not a reason to ratify**.

**On the merits: `SetStaticPrefix` was already RATIFIED at `qa/TASK-424.md` ruling 1**, and I am not
re-opening a prior gate's ruling. **455's CALLER, which is mine to rule on, is CORRECT independently:**
it gates on `IsReady()`, registers `GetCachedZoneA()` — **the same string `ComposeTurnPrompt` sends**,
which is what makes the plugin's `Prompt.StartsWith(Prefix)` pre-filter actually engage — refuses to
latch on a transient refusal, and latches the **warning** separately so a retry cannot spam.
✅ **RATIFIED.** No §8 rewrite is owed and nothing routes to the manager on this item.

## 8. TASK-456's TWO ITEMS

### (a) The 12 added lines against a "NOTHING ELSE" spec — ✅ **ALLOWED**

Verified at the artifact: the Charge arm carries a 10-line comment (`SiegeAssistantComponent.cpp:1493-1502`),
a `Log`-level success line (`:1503-1504`) and the call (`:1505`); Fallback carries a 1-line comment
(`:1509`), its log (`:1510-1511`) and its call (`:1512`).

**The mechanism is citable and I checked all three pins myself:** `CONVENTIONS.md:823`,
`SiegeAssistantCommand.h:59-60` and `SiegeAssistantSnapshot.cpp:674` **all still spell the seam
`SetUnitCommand`.** A future reader diffing the code against §8's seam sees a mismatch and "fixes" it
back — re-opening this defect a third time. The comment is a **revert defence against a threat that
exists on disk**, not decoration. The `Log` line matches the file's own success idiom verbatim
(`:1577`, `:1621`, `:1644`), and the `Warning → Log` drop is part of the fix: `Warning` was correct only
while the branch was defective. **Deleting the log would have left `charge` the only executor arm that
succeeds silently, in a batch whose smoke test reads this log.** ⇒ **Keep all 12 lines.**

### (b) `Executed` reported for a match-ended no-op — ✅ **RULED: ONE UNIFORM FOLLOW-UP TASK**

Confirmed at the artifact, not relayed: `ApplyArmyWideStance` returns silently on `bMatchEnded`
(`SiegePlayerController.cpp:1028-1031`); `ExecutePendingCommand` returns `true` unconditionally on both
arms (`:1506`, `:1513`); `ExecuteAndReport` then pushes `ESiegeAssistantReasonCode::Executed`
(`SiegeAssistantComponent.cpp:1331`). **Reachable window is real** — the deferred path can fire up to
`DeferredIntentTTLSeconds` after the sentence was typed.

⛔ **AND I VERIFIED THE PROGRAMMER'S CONTAINMENT CLAIM MYSELF RATHER THAN ACCEPTING IT:** every
`bMatchEnded` reference in `SiegePlayerController.cpp` is at `:647 :696 :855 :1028 :1127 :1325 :1332
:1457 :2013 :2497 :4162 :4212 :4228`. **`CreateUnitGroup` spans `:2844-2960` and contains none of
them.** ⇒ **`send`/`guard`/`ambush` already form groups post-match and already report `Executed`.**
The defect is **not confined to 456's two branches**, so a two-line patch there would make the executor
**less** uniform. ⇒ **456 was right to decline it. This wants one follow-up task covering ALL executor
arms uniformly** (`HasMatchEnded()` is public at `SiegePlayerController.h:647`). **Recorded as WARN-4,
not as a blocker against 456.**

## 9. TASK-454's DIFF — **THE ITEMISATION, NOT THE NUMBER (§18a)**

⛔ **I did not check the deletion count, and checking it would have failed this task wrongly.**
`757/94 → 836/108` is **correct for an extraction**: the header is 100 % additive (deletions unchanged
at 10) and all +14 `.cpp` deletions are moves. **Verified at their new homes:**

- the 4-statement shared body (`if (bMatchEnded) return;` · `CancelGroupPick()` · `ClearAllUnitGroups()`
  · `SetUnitCommand(NewCommand)`) is at **`SiegePlayerController.cpp:1028-1034`** — same guard first,
  same order, release before latch, argument parameterised;
- `OnCmdAttackPressed` at `:1037-1047` and `OnCmdDefendPressed` at `:1080-1088` each reduce to **one
  call**, with their **original comment blocks intact and appended to, never rewritten**;
- `ApplyArmyWideStance` has **no callers other than the two handlers and the assistant's two arms**.

⇒ **Provably zero behaviour change on the key path. CRITERION (p) MET.**

## 10. THE REMAINING BOARD CRITERIA

- **(b) NO MULTI-TURN LOOP — confirmed.** `DispatchTurnToModel` takes a **finished** prompt and grammar
  and has no route to a zone builder (`:1180-1183`); the completion lambda passes raw output **straight
  into a parameter** (`:1173-1178`) with no member to store it in. `ComposeTurnPrompt` (`:2604-2629`)
  is Zone A + B + C only, and `BuildPendingLine()` (`:2485-2540`) **takes no parameters** — its only
  inputs are `PendingArgs` (uint8/int32/FName) and `PendingReason` (an enum). **There is no signature
  through which model text could re-enter a prompt.**
- **(c) `CreateUnitGroup` extraction — confirmed zero-behaviour-change.** Read the caller
  (`:2772-2835`) against the extracted body (`:2844+`): id taken **before** the alive filter (`:2874`),
  field-for-field assignment, byte-identical alive re-filter, log→`BroadcastRefusal`→`CancelGroupPick`
  order preserved on the empty path, marker-ref nulls correctly left in the caller (`:2826-2827`).
  **Ownership is safe on the `INDEX_NONE` path**: `NewGroup` is a local never appended, so the caller
  still owns both decals and `CancelGroupPick` destroys them — **no double-free, no dangling.** The one
  reorder (the formation `UE_LOG`) is inert: it reads neither decal member.
  ⚠️ **No foreign-lane findings:** the FOLLOW lane is committed at `e70f5ba`, so nothing in these files
  is charged to this batch's loop budget.
- **(d) The guard — confirmed.** `ValidateCommandAgainstSnapshot`
  (`SiegeAssistantSnapshot.cpp:684-766`) refuses a mixed selection **as a whole**, naming the **first**
  offender in `Kinds` order, and drops nothing silently. ⛔ **It exposes the ORDERABLE tally, not the
  followable one** — traced to the fill site: `Entry->Orderable` is incremented inside the
  `Unit->IsGroupCommandEligible()` branch (`:488-503`), and `GetOrderableCount` reads `KindOrderable`
  (`:628-641`), which is filled from `Tally.Orderable` (`:577`). `KindFollowable` is a separate column
  and is untouched. The `KindNotOrderable` check is scoped to Send/Guard/Ambush via
  `SiegeAssistantGuardInternal::IntentTakesZoneOrder` (`:676-681`), **which is why "clerics follow me"
  still passes** — that scoping decision is correct and is the most valuable judgement in TASK-441.
  Test 5 (`…Guard.EmptyRoster`) and test 8 (`…Guard.ContractDetails`) exercise the `0 orderable` case,
  not merely the absent-kind case.
- **(e) NO PLAYER-FACING STRING FROM THE MODEL — confirmed.** Every outcome routes to an **existing**
  reason code (`AskUnsupported` / `AskWhichPlace` / `AskHowMany` / `ConfirmPrompt` / `Executed`). The
  confirm summary is `DescribeCommandForPlayer(PendingArgs.Command)` — assembled from a struct pinned
  to uint8/int32/FName.
- **§8/§9 pinned signatures — character-for-character.** `IsKindOrderable(FName) const` /
  `GetOrderableCount(FName) const`; `ValidateCommandAgainstSnapshot` takes
  `const TArray<FSiegeAssistantRosterEntry>&` (**not** the snapshot object — the property that keeps it
  testable with no world); `enum class ESiegeAssistantRejectReason : uint8 { None, KindNotOrderable,
  KindUnknown }`; `AttachConsoleWidget(USiegeAssistantConsoleWidget*)` at
  `SiegeAssistantComponent.h:723`. **No pin was "improved."**
- **`Build.cs` — confirmed.** `"SiegeLlama"` added (`:78`) and **nothing else**: ⛔ **no `HTTP`, no
  `Sockets`** — verified by reading the whole `PublicDependencyModuleNames` list. `SlateCore` **kept**
  (`:48`) with its example re-pointed from the deleted probe to `SiegeAssistantConsoleWidget.{h,cpp}`
  (`:34-47`), which is §12's owed edit exactly.
- **Input probe deleted — confirmed.** No `SiegeAssistantInputProbe.*` exists on disk.
- **M8 declaration — verbatim in all TEN handoffs.** Checked each: *"adds no replicated property, no
  new replicated class, no new relevancy tier."* ✅ **No "tier not declared" failure.**
- **§4 — no unit registry, actor cache, dirty flag or subscription list.** `CountLiveUnitsOfKind` and
  the selector are on-demand filtered `TActorIterator` passes kept nowhere.
- **(m) DEV-11's re-ask branch — ✅ RULED CORRECT, ON THE BRANCH'S LOGIC ALONE.**
  `EnterDeferredIntent:1829-1846`: if the threshold is already satisfied at latch time it does **not**
  latch and does **not** fire — it re-asks through the existing `AskHowMany`. ⛔ **I cite no eval run in
  either direction**; DEV-11 is §12h's least stable row and an observation could not settle this. On
  the logic: `at_least` is absolute and cannot express *"more"*, so an already-satisfied threshold is
  exactly what a misread relative reading looks like, and **re-asking is the fail-safe direction** — it
  costs a keypress; firing blind costs an army. ✅ **Keep the branch.**
- **(n) 443's stale board read on TASK-450 — NOT a finding, no loop opened.** The `#include
  "SiegeLlamaSubsystem.h"` resolves: the file exists and carries the referenced symbols. TASK-450
  landed.
- **(g-2) WARN-436-2 containment — VERIFIED AGAINST THE ARTIFACT, NOT THE HANDOFF.** The change is
  confined to `FSiegeSettingsSaveLoadRoundTripTest`'s body (`SiegeSettingsTest.cpp:323-394`): +1
  statement (`:383`), +1 pre-condition (`:384-385`), 1 moved statement (`:387`). Every occurrence of
  `SecondReader` in the file falls inside that function — **containment is provable from the file, not
  from the claim**. And the fix is real: driving the reader to `false` first is the only way the final
  `true` can have come from the slot. ✅ **The ratified overrun is contained.**
- **WARN-437-1 — landed correctly.** `Initialize();` is the first statement of
  `USettingsMenuWidget::RebuildWidget()` (`SettingsMenuWidget.cpp:92`), **before** tree construction,
  and it is idempotent and public. It does not disturb the corrected build order.
- **(g) TASK-451's sweep — NOT accepted as a raw count.** I re-derived the denominator: comparator
  lines per file are Grammar **53**, Guard **33**, ZoneA **14**, Settings **15** = **115**, which
  reconciles **exactly** to 451's 113 `TestEqual`-family sites + 1 `TestNotEqual` + 1
  `TestNotEqualSensitive`. ✅ **113 is correct and the board's 117 was wrong.** ✅
  **`SiegeAssistantZoneATest.cpp` is untouched and needed nothing** — its 5 string claims were already
  `*Sensitive`; **that is a credit to TASK-423, not a gap.** ✅ **Guard #2 discharged at the artifact:**
  `SiegeAssistantGrammarTest.cpp:464` now reads
  `TestEqualSensitive(TEXT("Symbol casing does not change the emitted grammar"), FromMixedCase, First);`
  against the mixed-case fixture at `:460` — and the guarantee under test is `.ToLower()` in
  `SiegeAssistantGrammar.cpp`, so with `TestEqual` the assertion **could not fail for the property it
  names**. The conversion demonstrably bites. ✅ **The engine mechanism is cited from engine source, not
  from the spec:** `AutomationTest.h:1997-2000` (thin forward) · `AutomationTest.cpp:2163`
  (`FCString::Stricmp`) · `:2295` (`FCString::Strcmp`). ✅ **Integers were not churned** — the 50
  remaining `TestEqual` sites I sampled are all `.Num()` / `int32` / `static_cast<int32>`. ✅ **Both
  deliberate `TestNotEqual` non-conversions are the calls I would have made** — `SiegeSettingsTest.cpp:132`
  in particular, where the insensitive comparator is the **stronger** one because Windows filenames are
  case-insensitive and two case-variant slot names are **the same file**.

---

## Findings

### BLOCKERS — **none**

### WARN

- **[WARN-1] TASK-451 — the `WITH_CASE_PRESERVING_NAME` residual is UNDERSTATED, and the number is
  about to be carried into TASK-447.** `handoffs/TASK-451-programmer.md` §5.2 says *"the **12**
  converted sites comparing `FName::ToString()`"* and then **enumerates 18 line references** (Grammar
  `:786,788,805,807,809,847,861` = 7 · Guard `:228,273,339,352,367,381,383,416,547` = 9 · Settings
  `:126,415` = 2). I sampled and confirmed the enumerated sites are genuine `FName::ToString()` casing
  comparisons — `SiegeAssistantGrammarTest.cpp:786`, `:788`, `:805`, `:807`, `:809` and
  `SiegeAssistantGuardTest.cpp:228-229` all compare `<FName>.ToString()` against a lowercase `FString`
  literal. ⛔ **The prose figure is wrong in the direction that under-states the exposure**, and this
  gate's own criterion (g-1) repeats it. **Fix: correct the count to the enumeration before TASK-447
  cites it.** ⚠️ **The RISK CLASS is unchanged and is correct as designed**: `WITH_CASE_PRESERVING_NAME`
  follows `WITH_EDITORONLY_DATA` = 1 under these `EditorContext` tests, **0 in a packaged build**, so
  **those assertions test the EDITOR, not the shipped game.** ⛔ **A green editor run at TASK-447 must
  NOT be reported as a shipped-build guarantee.** Carried forward, not closed.

- **[WARN-2] `SiegeAssistantComponent.cpp:1500-1501` — TASK-456's own deletion-verification sentence is
  false as written.** Its §3 states *"Post-edit, the words 'divergence', 'does NOT run', and 'standing
  group orders survive' appear nowhere in this file."* **Raw `Read` of `:1500` shows
  `// ⛔ Do NOT "restore" SetUnitCommand here: that re-opens the divergence` and `:1501` shows
  `// where an assistant \`charge\` left standing group orders parked and the`.** ⛔ **The DELIVERABLE is
  intact** — both lines are in 456's own *new* anti-revert comment, both statements are **true**, and
  all four false artifacts really are destroyed. But the verification sentence is the check a reviewer
  is invited to re-run, and re-running it **produces a hit**. **Fix: restate the claim as "no
  false-divergence assertion survives", or re-word the new comment.** Documentation only.

- **[WARN-3 · PLUGIN] `SiegeLlamaSubsystem.cpp:1557-1559` — the new prefill-timeout log prints the
  CEILING in an "elapsed" field, discarding the one measurement that matters.** The format string reads
  `"HARD TIMEOUT during PREFILL: %.1fs elapsed after %d of %d prompt tokens…"` and the argument
  supplied is `USiegeLlamaSubsystem::HardTimeoutSeconds` — **the constant 10.0, not the measured
  elapsed time.** The firing condition is `FPlatformTime::Seconds() > HardDeadlineSeconds` (`:1550`),
  so true elapsed is ≥ 10.0 and may exceed it by up to one `llama_decode` slice. ⚠️ **That overshoot is
  precisely the quantity the header promises is bounded** (`SiegeLlamaSubsystem.h:141-147`), and this
  is the only line that could ever measure it — so a large overshoot would be invisible and would print
  as exactly `10.0s`. ⚖️ **This is the same shape TASK-457 was fixing** (a stated bound standing in for
  a measurement). **Fix: pass `FPlatformTime::Seconds() - StartSeconds`.** One argument. Not blocking:
  the control flow, the flag choice and the guard are all correct.

- **[WARN-4] `SiegeAssistantComponent.cpp:1506` / `:1513` + `SiegePlayerController.cpp:1028` — the
  executor reports `Executed` for a match-ended no-op, and it is NOT confined to `charge`/`fallback`.**
  Full trace and my verification of the containment claim are in §8(b) above. ⛔ **`CreateUnitGroup`
  (`SiegePlayerController.cpp:2844-2960`) contains ZERO `bMatchEnded` references — I checked every
  occurrence in the file — so `send`/`guard`/`ambush` already have this shape today.** ⇒ **RULING: one
  uniform follow-up task across ALL executor arms, not a patch in 456's two branches.** `HasMatchEnded()`
  is public at `SiegePlayerController.h:647`, so it is cheap whenever the manager wants it. §17's shape,
  named by the programmer rather than left to be found — **credit, not a defect against 456.**

- **[WARN-5] TASK-449 §8 — a console left OPEN when the match ends stays on screen over the victory
  screen.** Verified it is **consistent, not a soft-lock**: `CanOpenAssistantConsole()`
  (`SiegePlayerController.cpp:4212`) refuses to *open* after match end, `ApplyCursorInputState`
  early-outs while `bMatchEnded` is latched so `HandleMatchEnd`'s `UIOnly` stands, and the key still
  closes it. **A one-line close in `HandleMatchEnd` would tidy it.** Outside 449's "you are writing the
  JOIN, nothing else" scope. ⇒ **Jonathan's call at TASK-448**, already on 449's playtest checklist.

- **[WARN-6] TASK-443 §7(c) — two paired tunables duplicate protected controller members.**
  `AssistantPositionRadius = 700` ≡ `GroupPositionRadiusDefault` and `AssistantAttackRadius = 1500` ≡
  `GroupAttackRadiusDefault`, used at `SiegeAssistantComponent.cpp:1433-1434`. Both controller members
  are `protected` and unreachable. **Declared as paired tunables rather than hard-coded, so the second
  source of truth is NAMED** — the shipped `SpawnBoxHalfExtent` 3-way precedent. ⛔ **Keep in lockstep.**
  Clean fix = one access change on the controller, then delete the copies. Follow-up.

- **[WARN-7] `IntentTakesZoneOrder` exists file-locally in TWO translation units and they MUST agree.**
  `SiegeAssistantSnapshot.cpp:676-681` (`SiegeAssistantGuardInternal`) and the component's own internal
  namespace. **I verified both return Send | Guard | Ambush today.** Both are file-local statics, so
  neither can reach the other; the shared home would be the pinned `SiegeAssistantCommand.h`. ⛔ **If
  one ever gains a verb and the other does not, the guard and the executor disagree about what a zone
  order is — silently.** Consolidation is owed to whoever next owns that header.

- **[WARN-8] TASK-443 §7(b) — an assistant-formed group owns NO persistent ground markers.**
  `ExecuteZoneOrder` passes `nullptr, nullptr`, because `ConfirmPressed` tears the ghosts down
  **before** executing (`:761`) — correctly, so no ghost outlives its decision, but leaving nothing to
  transfer. **Verified safe**: no double-free, no dangling (TASK-440 contract #2 is ownership-transferring
  and receives nothing). ⚠️ **Visibly different from a key-formed group** — an assistant `send` leaves
  no circle on the ground where R/F would. **Feel item for TASK-448**, not a defect.

- **[WARN-9] TASK-455 §2d — 4 surviving `MaxSnapshotChars` doc-comment mentions vs §8's literal *"a
  grep returns nothing."*** ✅ **RULED: ALLOW.** Verified **zero code and zero log-string references**;
  the four (`SiegeAssistantSnapshot.h:206`, `:208`, `:210`, `:352`) are the retirement notice itself
  plus one dated parenthetical. This is the file's own established dead-spelling idiom (the
  `llama_kv_cache_seq_rm` precedent) and it is the **§14-correct** behaviour: a reader arriving from an
  older doc and grepping the dead name lands on *"retired, here is the successor"* instead of on
  silence, which a negative search cannot distinguish from absence. ⛔ **Do not delete them.** Recorded
  as a WARN only because §8's literal wording and the shipped file now disagree — **the wording is what
  should move**, and that is a one-line manager amendment, not a code fix.

### NIT

- **[NIT-1] `handoffs/TASK-456-programmer.md:264-266`** ends with stray `</content>`, `</invoke>`,
  `</output>` tags — an authoring artifact. Harmless; strip on the next touch.
- **[NIT-2] Stale seam spellings, none FALSE, all outside 456's deliverable.** `CONVENTIONS.md:823` ·
  `SiegeAssistantCommand.h:59-60` · `SiegeAssistantSnapshot.cpp:674` · `SiegeAssistantComponent.h:182`,
  `:957` · `SiegeAssistantComponent.cpp:19` all describe the seam as `SetUnitCommand`, which is still
  literally what happens (as `ApplyArmyWideStance`'s last statement). ⚠️ **`CONVENTIONS.md:823` is the
  highest-value one — it is the text a future implementer would "restore" the code to match** — and it
  is the **manager's** file. 456 correctly left all five rather than burying its two-line deliverable in
  a sweep.
- **[NIT-3] `SiegeLlamaSubsystem.h:159` and `SiegeLlamaSpike.cpp:174` still name the retired game-lane
  constant.** Both are outside 455's write scope (the latter is 🔒 §16 protected). Plugin-lane one-liner.
- **[NIT-4] ⛔ FOUR STALE OFFSETS I DELIBERATELY DID NOT FILE AS FINDINGS (§18c).** This gate's criteria
  cite `SiegeLlamaSubsystem.h:149`/`:170` for the plugin constants — they are now **`:178`/`:199`**;
  `qa/TASK-424.md` cites `SetStaticPrefix` at `:286` — now **`:328`**; the board cites 456's call sites
  at `:1413`/`:1420` — now **`:1505`/`:1512`**; and 457's spec cites `:1344` for the between-slice test,
  which has moved. **Every one moved because a sibling task inserted lines above it. Nothing about any
  of them changed.** ⇒ **A criterion that fails only because a number moved has found nothing.** All
  four were verified by symbol and by relative order instead.
- **[NIT-5] `SiegeAssistantSnapshot.cpp:499-501`** carries a real latent hazard, correctly commented in
  place: `Entry` is a pointer into `Roster` obtained ~30 lines earlier and is valid **only** because
  nothing between the two touches `Roster`. **Anyone inserting a `Roster` write between them must
  re-fetch `Entry`.** Said in the code, which is the right place.
- **[NIT-6] TASK-441's new file `Siegebound/Tests/SiegeAssistantGuardTest.cpp` was declared, not
  slipped in**, and lands under the shipped `Siegebound/Tests/` precedent alongside
  `SiegeAssistantGrammarTest.cpp` and `SiegeSettingsTest.cpp`. ✅ **Correct home; no action.**
  Likewise `FSiegeAssistantRosterEntry::Orderable` — one appended `int32`, strictly additive, filled
  from an **already-computed** predicate result with no second traversal, and **no zone prints
  `Roster`**, so the Zone B/C byte freeze (§12c) is undisturbed. ✅ **Ruled in.**

---

## 11. ⚠️ PLUGIN APPENDIX — TASK-457 (CROSS-LANE, SCOPED)

⛔ **This section is scoped to `Plugins/SiegeLlama/` and does not dilute the game-lane verdict above.**
Files: `Private/SiegeLlamaSubsystem.cpp` + a **comment-only** change to `Public/SiegeLlamaSubsystem.h`.

### **VERDICT: PASS** — 0 blockers, 1 WARN (WARN-3 above)

- ⭐ **THE PLACEMENT IS CORRECT, AND I CHECKED PLACEMENT RATHER THAN PRESENCE.** The deadline test is at
  the **BOTTOM** of the prefill loop body — `SiegeLlamaSubsystem.cpp:1550-1561`, **after**
  `Offset += ChunkTokens` (`:1514`) and after `CheckSoftTimeout` (`:1518`) — while the pre-existing
  cancel/stop test sits at the **top** (`:1486`). ✅ **A top-of-body version would have been inert on
  the hot path**: with prefill ~347 tokens against `EffectiveBatchSize = llama_n_batch(Context)`
  (assigned at `:980`, 512 requested) the body runs **once**, so only a bottom test covers the last —
  and only — slice. **This is the check that would have passed a broken fix, and it passes correctly.**
- ⭐ **THE `|| bAborted` GUARD EXTENSION IS PROVABLY INERT ON EVERY PRE-EXISTING PATH — I DERIVED IT,
  I DID NOT ACCEPT IT.** `bAborted` is initialised `false` (`:1458`). Within the prefill loop it is
  written in exactly two places: `:1503` (`bAborted = (DecodeResult == 2)`), which is **always** paired
  with `bDecodeFailed = true` one line earlier at `:1500`, and the **new** `:1556`. The decode loop's
  writes (`:1681`, `:1696`) are **downstream** of the guard at `:1581`. ⇒ **Before `:1556` existed,
  `bAborted ⇒ bDecodeFailed`, so the new term could never decide the test. The claim holds.**
- ⛔ **THE REJECTION OF `qa/TASK-424`'s SUGGESTED ONE-LINE FIX IS RATIFIED, AND IT WAS WRONG TWICE.**
  (i) `|| ShouldAbortNow()` on the `:1486` test sets `bCancelled = true`, and the completion string is
  built by `bCancelled ? FString::Printf(TEXT("cancelled -- %s"), …) : FailureDetail` (`:1590`) — so a
  **timeout would have been reported to the player as a cancel nobody performed.** (ii) It sets
  **neither** flag the post-prefill guard tested, so the run would have **fallen through into the
  sampler with a half-prefilled prompt** and emitted a fluent, grammar-valid unit order derived from a
  truncated one. ⚠️ **That is the worst failure mode this feature has** and it is strictly worse than
  the defect being closed. Using `bAborted` — **the same flag the decode loop's own timeout already
  uses** (`:1696`) — plus the guard extension is the correct shape. 📌 **RECORDED AS LAW-WORTHY: a
  reviewer's suggested fix carries reviewer authority but has not been traced through the code.** It is
  the same family as every trap this wave has found, and it fired on **my own lane's prior report**.
- ✅ **WARN-1 FIXED, NOT JUSTIFIED — and the declared reorder is RATIFIED.** `MarkBusy()` now refuses
  unless `Idle` (`:416-423`), symmetric with `ClearBusy` (`:386-393`); `Run()`'s `bWorkPending` branch
  re-tests `GetState() == EState::Busy` (`:639`) and **completes the request on the else branch**
  (`:645-668`), which is what keeps `RequestCompletion`'s *"a true return means OnComplete fires exactly
  once"* true through the new path — **without it the FSM strands in "thinking" for the match with no
  error and no log.** ⇒ **On the reorder (`MarkBusy()` at `:340` now BEFORE `bWorkPending = true` at
  `:350`): RATIFY.** The mechanism is checkable and I checked it — the run loop's wait is bounded at
  **200 ms** (`:613`), so the worker ticks unprompted; with the flag raised first it could observe
  pending work, find the state still `Idle`, and **refuse a perfectly healthy request.** Publishing
  `Busy` first makes the window **empty by construction**. It exceeds the literal "two lines" and the
  guard is **not safe without it**. 📌 And the function's own comment had documented this order all
  along while the code did the opposite — §17 one function away.
- ✅ **CROSS-LANE ORDER PRESERVED — confirmed, because TASK-455 depends on it.**
  `ApplyPendingStaticPrefix()` (`:626`) still precedes the `bWorkPending` branch (`:628`). A condition
  was added **inside** the branch; the branch was not moved.
- ✅ **WARN-3 (VRAM) is a citation re-point only.** `VramGameReserveMiB = 768` **unchanged** at `:104`;
  the doc now cites `handoffs/TASK-413-buildmaster.md:625` (*"+2.0 to +4.6 ms on EVERY TIER"*) instead
  of the full-tier `:638` for a partial-tier bar. ✅ **Conclusion survives; the number was never the
  problem.**
- ✅ **BOTH FENCES HELD.** `HardTimeoutSeconds = 10.0` verbatim at `SiegeLlamaSubsystem.h:161`. The
  header's **`BACKSTOP` promise was NOT softened** — `:133-147` keeps the word and adds the mechanism,
  the granularity, and **two explicit non-claims** (`:145` no mid-graph cut on the GPU tiers; `:149-153`
  the clock starts on the worker, not at the keypress, so this bounds **inference** time and not the
  player's end-to-end wait). ⚖️ **The code was changed to match the claim, not the claim to match the
  code** — the required direction.
- ✅ **THE SELF-DECLARED THIRD OVER-CLAIM IS CORRECT AND THE CORRECTION IS RIGHT.** The *"at most one
  ubatch"* bound was wrong by **8×** in the flattering direction. **Mechanism verified without trusting
  the refuser:** the loop calls `llama_decode` with `ChunkTokens = min(EffectiveBatchSize, remaining)`
  (`:1495`) and `EffectiveBatchSize = llama_n_batch(Context)` (`:980`, 512 requested); `UBatchTokens = 64`
  sizes graphs llama builds **internally inside one call**, and **the worker never returns between
  them.** ⇒ **A loop cannot bound anything at a boundary it does not return through.** The guarantee
  survives — bounded, not unbounded; only the number was wrong. Corrected in both owned files;
  `handoffs/TASK-450-programmer.md` §4 still carries the old figure and is **not 457's to edit**.
- ✅ **SCOPE FENCE HELD:** no queue, no threading-model change, no game-lane edit, no `Content/`, and
  🔒 **`SiegeLlamaSpike.cpp` UNTOUCHED** (§16) — its similar wording says **"ADVISORY"**, not
  "BACKSTOP", which is accurate for the spike.

### ⛔ AND THE ONE THING NOBODY MAY REPORT AS A PASS

**Absence of the new `HARD TIMEOUT during PREFILL` log line is NOT evidence the fix works — it is
evidence the prefill was fast.** Measured prefill is ~1.7 s on partial, so the 10 s ceiling is
**unreachable there**. ⇒ **The only reachable exercise is the CPU tier**, where TASK-413 measured **2 of
5 generations hitting the ceiling**. ⛔ **Anyone claiming this fix verified without a CPU-tier run is
claiming a measurement they did not take.** ⚠️ Likewise the WARN-1 refusal branch: it needs
`LatchFaulted` during `ApplyPendingStaticPrefix` on the same iteration as a dispatch, which today's
Zone A (1683 < 2048) **cannot produce** — reachable by construction, not on today's numbers. Its
correctness is a **reading** argument and it must be reported as one.

---

## Notes for build-master (TASK-447)

1. ⛔ **NOTHING BELOW IS "PASSING." Everything in this report is a claim about SOURCE.** The compile and
   the automation run are the first parsers to see any of it, and they are the reviewers of record.
2. **Link-order constraint, stated loudly:** `USiegeAssistantComponent::AttachConsoleWidget` is called
   from `SiegePlayerController.cpp:4348` and **defined** at `SiegeAssistantComponent.cpp:2127`. **Both
   are present now** — I verified the definition on disk. No loop is owed on it.
3. **Expected first failures, in order, and none of them is a QA miss:** the **three `static_assert`s**
   at `SiegeAssistantComponent.cpp:52/:59/:71` (they hold on today's values by hand: 1085<3000 ·
   192<1085 · 480<893) · `TestEqualSensitive` overload resolution on all 58 converted sites ·
   UHT on the 2 new delegates, 2 new `BlueprintAssignable` `UPROPERTY`s and 4 new `UFUNCTION`s in the
   component, the 4 new reflected members in the controller, and the 12 `UFUNCTION`s + 6
   `BindWidgetOptional` members in the console widget. **None has been through UHT.**
4. ⛔ **`git diff` IS THE WRONG INSTRUMENT ON THIS BATCH.** `SiegeAssistantComponent.{h,cpp}`,
   `SiegeAssistantConsoleWidget.{h,cpp}`, `SettingsMenuWidget.*`, `SiegeSettingsSaveGame.*`, the test
   files and **both plugin files** are untracked (`??`), so plain `git diff` shows nothing and **reads
   as "changed nothing"** — §14 instance 3. TASK-456 found this and used `git diff --no-index` against
   a byte-exact snapshot; ✅ **I accept that as the correct substitute and recommend it for any
   verification you do.**
5. **First-execution audit — three readings this gate has made takeable and pinned on you:**
   (i) `ReportFirstCapture` (`SiegeAssistantComponent.cpp:2720`) now prints `ZoneBCharReserve`, the live
   `zoneB_chars` and the over-charge, with the sizing instruction attached — **that is the owed
   `ZoneBCharReserve` reading, and it has never executed.** (ii) TASK-450's three inertness Warnings
   should **stop firing** and `LogSiegeLlama`'s `STATIC PREFIX REGISTERED: N chars -> N tokens` should
   **appear** — that line is the shipped lane's **first ever measured `zoneA_tok`**, and it discharges
   §12g's standing WARN without the spike. (iii) TASK-441's cheap check: `sum(Roster[kind].Orderable)`
   must equal the `orderable=` figure the roster line prints for that kind — **both derive from the
   same per-unit call, so they must agree exactly.** ⛔ **An audit that reports "no problems" without
   stating which of these it observed has not been performed.**
6. ⛔ **Do NOT report a green editor automation run as a shipped-build guarantee** — see WARN-1. The
   `FName` casing assertions test the **editor** (`WITH_CASE_PRESERVING_NAME` = 0 in a packaged build).
7. ⛔ **Do not score TASK-457 by the silence of its new log line** — see the plugin appendix's closing
   clause. The reachable exercise is the CPU tier.
8. **`SiegeLlamaSpike.cpp` stays** (§16) and **`Docs/Data/assistant_eval_holdout2.csv` stays sealed** —
   nothing in this batch reads, scores against or quotes it.
9. ⛔ **Nothing in this batch is accuracy progress.** The eval scores **emitted JSON**; this batch
   changes what the game **does**. **Bar #5 stands at 20/25 against 22 + zero refuse-class failures —
   uncleared.** Wave 1 is un-gated because **Jonathan weighed the failure modes and overrode his own
   gate**, never because the ladder worked.

## Board flips owed (orchestrator to apply — I do not edit the board)

`TASK-440 · 441 · 443 · 444 · 449 · 451 · 453 · 454 · 455 · 456 · 457` → **`qa-passed` /
`ready-for-integration`** · `TASK-446` → **`qa-passed`**.

## Follow-up tasks this gate recommends (manager's to create)

1. ⛔ **Uniform match-ended reporting across ALL executor arms** (WARN-4) — the one with real
   consequence.
2. **Correct the `FName` residual count** in `qa/TASK-446.md`'s WARN-1 line and in TASK-447's criteria
   before either is cited (WARN-1).
3. **One-argument fix to the plugin's prefill-timeout log** (WARN-3).
4. **Take the `ZoneBCharReserve` reading** from `ReportFirstCapture` — needs the editor + a compiled
   build and nothing else.
5. **Manager-owed doc amendments:** `CONVENTIONS.md:823`'s seam spelling (NIT-2); §8's *"a grep returns
   nothing"* wording vs the ratified dead-spelling idiom (WARN-9); §13's `FText`/`FName` reach, which
   TASK-451 §1b re-derived at engine source.
6. **Consolidate `IntentTakesZoneOrder`** into `SiegeAssistantCommand.h` (WARN-7) and **widen the two
   controller radius members** so 443's paired tunables can be deleted (WARN-6).

---

# ⛔ APPENDED BY BUILD-MASTER 2026-08-03 — TASK-447 COMPILE GATE: **FAILED**. QA LOOP 1 OPENED ON **TASK-444 ONLY**.

**This section is appended, not edited in. Nothing above it was altered.** Author: build-master. I did
**not** fix any code — that is the programmer's, per the pipeline's separation of duties.

## Verdict, quoted from the log (`scratchpad/build2.log`)

```
Result: Failed (OtherCompilationError)
Total execution time: 14.21 seconds
```

⛔ **`$LASTEXITCODE` was `6` and was NOT used.** The log is the authority (§17: `Build.bat` lies in both
directions). ✅ **This is a REAL failure with REAL diagnostics** — unlike the first attempt, which was the
Live Coding mutex and named no file.

## ⚖️ ATTRIBUTION — 7 of the 8 diagnostics are TASK-444's. **The other lane is TASK-450's and is filed in `qa/TASK-424.md`, not here.**

**A failure belongs to the file the diagnostic names.** Only **two** translation units failed.

### ⛔ TASK-444 — `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantConsoleWidget.cpp` — 7 errors

**(1) `error C2445` × 2 — ambiguous ternary. Both are the `TObjectPtr`/raw-pointer conversion trap.**

```
SiegeAssistantConsoleWidget.cpp(72,74): error C2445: result type of conditional expression is ambiguous:
types 'TSubclassOf<USiegeAssistantConsoleWidget>' and 'UClass *' can be converted to multiple common types
        ConsoleClass ? ConsoleClass : USiegeAssistantConsoleWidget::StaticClass();
                                                                               ^
```
```
SiegeAssistantConsoleWidget.cpp(207,63): error C2445: result type of conditional expression is ambiguous:
types 'UVerticalBox *' and 'TObjectPtr<UVerticalBox>' can be converted to multiple common types
    UVerticalBox* ContentParent = (Column != nullptr) ? Column : RootPanel;
                                                                 ^
```

**(2) `error C4458` × 5 — a local named `Slot` shadows `UWidget::Slot`.** ⚠️ **C4458 is normally a
WARNING; this module builds warnings-as-errors, so it is fatal here.** All five are the same shape:

| site | the shadowing declaration |
|---|---|
| `:220,26` | `if (UVerticalBoxSlot* Slot = ContentParent->AddChildToVerticalBox(TranscriptText))` |
| `:240,26` | `if (UVerticalBoxSlot* Slot = ContentParent->AddChildToVerticalBox(StatusText))` |
| `:259,26` | `if (UVerticalBoxSlot* Slot = ContentParent->AddChildToVerticalBox(InputBox))` |
| `:302,28` | `if (UHorizontalBoxSlot* Slot = ButtonRow->AddChildToHorizontalBox(ConfirmButton))` |
| `:322,28` | `if (UHorizontalBoxSlot* Slot = ButtonRow->AddChildToHorizontalBox(CancelButton))` |

Engine side of every one of the five, quoted from the log:
`Engine/Source/Runtime/UMG/Public/Components/Widget.h(264,25): note: see declaration of 'UWidget::Slot'`
→ `TObjectPtr<UPanelSlot> Slot;`

⇒ ⚠️ **Because `USiegeAssistantConsoleWidget` derives from `UUserWidget` → `UWidget`, `Slot` is an
inherited member and any local of that name inside a member function shadows it.** ⛔ **Fix is the
programmer's call, not mine** — but note the shape is uniform, so §22 applies: **sweep for the shape,
do not patch the five cited lines and stop.** (I checked: no other file in the batch declares a local
`Slot`; these five are the extent **as of this build** — and a build only reports what it reached.)

## ✅ WHAT THIS BUILD PROVES **IN TASK-444's FAVOUR**, AND IT IS NOT NOTHING

⛔ **Do NOT re-open settled findings on the back of this failure.** Measured at this run:

- ✅ **UHT ACCEPTED THE ENTIRE BATCH.** `Module.GitClaudeUnrealTest.{1..5}.cpp` and `Module.SiegeLlama.cpp`
  all compiled (actions 9, 14-18 of 23). ⇒ **Every `UFUNCTION` / `UPROPERTY` / delegate / `BindWidgetOptional`
  UHT caveat declared across the handoffs is DISCHARGED** — including 444's own 4 delegates, 5 BIEs,
  12 `UFUNCTION`s and 6 `BindWidgetOptional` members. **The reflection surface is valid.**
- ✅ **16 OTHER TRANSLATION UNITS COMPILED CLEAN**, including `SiegeAssistantComponent.cpp`,
  `SiegePlayerController.cpp`, `SiegeAssistantSnapshot.cpp`, `SettingsMenuWidget.cpp`,
  `SiegeSettingsSaveGame.cpp`, `SiegeSettingsSubsystem.cpp` and **all four test TUs**.
  ⇒ **TASK-440 · 441 · 443 · 449 · 451 · 453 · 454 · 455 · 456 (and 423 · 436 · 437) produced no
  diagnostic.** ⛔ **None of them is in a QA loop and none of their statuses changes.**
- ✅ **TASK-451's `TestEqualSensitive` sweep COMPILES** — all four test TUs built clean, so the 58
  converted assertions resolve to real overloads. ⚠️ **Compiling is not passing; they still have never run.**

## ⛔ WHAT THIS BUILD DOES **NOT** PROVE — stated so nobody over-reads it

1. ⛔ **THE LINK STEP NEVER RAN.** The build stopped after 18 of 23 actions. **No unresolved-external
   evidence exists in either direction.** My pre-flight found all 22 cross-task definitions present
   (`handoffs/TASK-447-buildmaster.md` §5), but **that is a prediction, not a measurement.**
2. ⛔ **THIS IS NOT NECESSARILY THE COMPLETE ERROR SET.** A failing TU stops at its own errors, and
   5 actions never started. **More diagnostics may surface behind these.** ⚖️ **"Fix these 8 and it
   builds" is NOT a claim this report makes.**
3. ⛔ **NOTHING EXECUTED.** No test ran; `USiegeAssistantSnapshot` still has **zero executions**;
   **WARN-5 remains INSTRUMENTED, NOT DISCHARGED.**

## Board flips owed (orchestrator applies; build-master does not edit status)

- **TASK-444** → ⛔ **`qa-failed`** — 7 compile errors in its own file. **QA loop 1 of 3.**
- **TASK-450** → ⛔ **`qa-failed`** — 1 compile error, filed in **`qa/TASK-424.md`** (its gate), not here.
- **All other tasks in this gate:** ⛔ **NO CHANGE.** They compiled clean; a foreign lane's failure is
  never counted against them.
- **TASK-447** → remains open, blocked, **not `done`**. Nothing was committed.
