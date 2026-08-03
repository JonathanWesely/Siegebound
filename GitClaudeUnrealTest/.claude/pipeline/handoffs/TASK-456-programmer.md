# TASK-456 — gameplay-programmer handoff (2026-08-03)

**[W1-RELEASE-b] The assistant's `charge`/`fallback` now CALL `ApplyArmyWideStance` — the call side**

**M8 DECLARATION DUTY, stated verbatim as required:**
> "adds no replicated property, no new replicated class, no new relevancy tier."

- **Status:** `ready-for-qa` (reviewed under TASK-446)
- **File touched:** `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantComponent.cpp` — **and nothing else.** One function, one hunk.
- ⛔ **NOT compiled** (TASK-447 is the single gate). No Git, no editor, no MCP, no PIE, no `Content/`, no controller edit, no header edit.

---

## 0. ⛔ THE ONE THING TO READ IF YOU READ NOTHING ELSE

**The defect TASK-454 opened the door for is now CLOSED.** 454's own words were *"'can' is not 'does'."* — it built `ApplyArmyWideStance` and correctly did not touch this file. **Two lines now walk through that door**, and the divergence they documented is deleted rather than softened.

**Before:** an assistant `charge` latched the stance and left standing group orders parked; the T key with the same meaning freed them. **Same words, two outcomes.**
**After:** both reach **one implementation**.

⚠️ **ONE CONSEQUENCE NEEDS A RULING AND IT IS IN §6 — the new `bMatchEnded` guard can make the executor a silent no-op while the component still reports `Executed`.** I did **not** fix it: fixing it is a guard change, which my spec forbids, and it is not confined to my two branches.

---

## 1. THE STOP CONDITION — DISCHARGED, EXACTLY TWO CALL SITES

The spec: *"If you find a number of call sites other than two, STOP and report."*

**Found exactly two, by symbol, per §18c.** ⚠️ **The board's `:1413`/`:1420` were stale as predicted** — TASK-455 shifted this file by ~93 lines. I never used those offsets.

```
grep -n "SetUnitCommand" SiegeAssistantComponent.cpp   (PRE-EDIT, whole file)
  :19    include comment (prose)
  :1494  //  comment (prose)
  :1497  //  comment (prose)
  :1505  TEXT() log string (prose)
  :1506  Controller->SetUnitCommand(ESiegeUnitCommand::Attack);   <-- CALL 1
  :1512  TEXT() log string (prose)
  :1513  Controller->SetUnitCommand(ESiegeUnitCommand::Defend);   <-- CALL 2
```

⚖️ **7 matching lines, 2 call sites** — the §14-instance-2 shape (*a match is not a call site*), stated here because a reviewer counting matches would get 7 and think five sites went missing. **Both calls are in `USiegeAssistantComponent::ExecutePendingCommand`, on the `Charge` and `Fallback` switch arms. There is no third.**

**Sweep for other spellings, all negative and all confirmed on disk, not by search alone:** no `Controller.SetUnitCommand`, no `GetController()->SetUnitCommand`, no `ServerSetUnitCommand` — post-edit `grep -c "Controller->SetUnitCommand\|Controller\.SetUnitCommand"` = **0**.

---

## 2. THE TWO SITES BY SYMBOL, AND THEIR NEW CONTEXT

| # | anchor (symbol — durable) | line (hint only) | now reads |
|---|---|---|---|
| 1 | `ExecutePendingCommand` → `case ESiegeAssistantIntent::Charge` | `:1505` | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Attack);` |
| 2 | `ExecutePendingCommand` → `case ESiegeAssistantIntent::Fallback` | `:1512` | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Defend);` |

**The entry point is reachable — verified, not assumed:**

- `void ApplyArmyWideStance(ESiegeUnitCommand NewCommand);` at **`SiegePlayerController.h:474`**, inside `public:` (section opens **:184**, closes at `protected:` **:736**) ⇒ **public**.
- Implementation at **`SiegePlayerController.cpp:1006`**.
- This TU already includes `Siegebound/SiegePlayerController.h` (**:19**) — **no new include, no new coupling.**

---

## 3. ⛔ THE COMMENTS ARE DELETED, NOT SOFTENED — PROOF

**Four now-false artifacts were removed. Two were `//` comment blocks; two were `UE_LOG(..., Warning, ...)` statements carrying the same false sentence into the runtime log.** I deleted **both kinds**, because the log line is the worse of the two: a stale comment misleads the next reader, but a stale `Warning` misleads **Jonathan, at runtime, mid-battle**, asserting a defect that no longer exists.

| deleted | what it claimed | why it had to go |
|---|---|---|
| `Charge`'s 11-line `// ⚠️ FLAGGED DIVERGENCE FROM THE KEY…` block | *"The assistant's 'charge' latches the stance but does NOT release standing group orders"* | **false as of this task** |
| `Charge`'s `UE_LOG(…, Warning, …)` | *"Unlike the T key this does NOT run the TASK-344 release law (ClearAllUnitGroups is private), so standing group orders survive. Declared divergence"* | **false, and it printed at runtime** |
| `Fallback`'s `// Same declared divergence as Charge, for the E key.` | inherits the same false claim | **false** |
| `Fallback`'s `UE_LOG(…, Warning, …)` | the E-key twin of the above | **false, and it printed at runtime** |

**Post-edit, the words "divergence", "does NOT run", and "standing group orders survive" appear nowhere in this file.** ✅ **Not one of the four was reworded, downgraded, or kept in weakened form.**

### ⚠️ DECLARED ADDITION (§15) — QA MUST RULE ON IT, I AM NOT SLIPPING IT PAST YOU

Deleting is not the same as leaving a vacuum, and I did **not** leave a vacuum. I added, at the Charge arm, **a 10-line comment and one `Log`-level success line**, plus a 1-line comment and one `Log` line at Fallback. **My spec says "NOTHING ELSE", so this is a declared departure and here is the checkable mechanism:**

**(a) THE COMMENT EXISTS TO PREVENT THE EXACT REVERT THE SPEC IS AFRAID OF, AND THE THREAT IS CITABLE, NOT HYPOTHETICAL.** Three artifacts still pin the seam with the *old spelling*:
- `CONVENTIONS.md:823` — *"`charge`/`fallback` → `SetUnitCommand(Attack/Defend)`"*
- `SiegeAssistantCommand.h:59-60` — same
- `SiegeAssistantSnapshot.cpp:674` — same

⇒ **A future reader diffing my code against §8's seam sees a mismatch and "fixes" it back** — re-opening this defect for the third time. The comment says, in terms: *§8's seam is HONOURED, `SetUnitCommand(Attack)` is still what happens, as `ApplyArmyWideStance`'s LAST statement.* ⚖️ **This is the spec's own stated reasoning for the deletion, applied in the other direction.**

**(b) THE `Log` LINE IS THIS FILE'S OWN HOUSE IDIOM, NOT AN INVENTION.** Every sibling executor branch already logs its success in one fixed shape — I matched it character-for-character:

```
:1577  TEXT("EXECUTED %s: group %d formed … through ASiegePlayerController::CreateUnitGroup - the SAME API the R/F keys call.")
:1621  TEXT("EXECUTED FOLLOW: %d unit(s) enrolled through …EnrollInDefaultFollowGroup - the SAME API the C key and the spawn auto-enrol call.")
:1644  TEXT("EXECUTED RALLY through AHeroCharacter::Rally() - the SAME API the IA_Rally key calls.")
```
⇒ mine: `TEXT("EXECUTED CHARGE through ASiegePlayerController::ApplyArmyWideStance(Attack) - the SAME API the T key calls, release law included.")`

**Deleting the log outright would have left `charge` the ONLY executor arm that succeeds silently** — an observability hole in a batch whose smoke test (TASK-447) reads this log.

**(c) THE SEVERITY DROP `Warning` → `Log` IS DELIBERATE AND IS PART OF THE FIX.** `Warning` was correct only while the branch was defective; every sibling success uses `Log`. **Leaving it at `Warning` would keep a defect-shaped signal on a now-correct path.**

⛔ **If QA rules the literal reading — bare swap, zero replacement text — deleting my 12 added lines is a two-minute edit and I will not argue.** It should be a ruling, not a drive-by.

---

## 4. DIFF EVIDENCE — AND WHY `git diff --stat` IS THE WRONG INSTRUMENT HERE (§14 instance 3)

⚠️ **`SiegeAssistantComponent.cpp` IS UNTRACKED (`??`) — `git ls-files` returns EMPTY, so `git diff --stat` CANNOT SEE IT AND SHOWS NOTHING.** Quoting the repo's `git diff --stat` would have shown my file absent and read as *"changed nothing"*. **That is the §14 trap, and quoting it uncritically would have been the defect this wave keeps finding.**

**I took a byte-exact pre-edit snapshot and produced a real diffstat with `--no-index`, the only form that can see an untracked file:**

```
$ git diff --no-index --stat <scratchpad>/SiegeAssistantComponent.cpp.BEFORE \
                             Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp

 .../Siegebound/SiegeAssistantComponent.cpp | 35 +++++++++++-----------
 1 file changed, 17 insertions(+), 18 deletions(-)
```

**File length: 2821 → 2820 lines (net −1).** ⚠️ **ONE HUNK ONLY — `@@ -1491,25 +1491,24 @@`.** The entire diff is inside `ExecutePendingCommand`'s `Charge`/`Fallback` arms. **No other region of the file appears in the diff at all.**

### §18a — "ADDITIVE" IS A CLAIM ABOUT BEHAVIOUR, AND THIS IS A **SWAP**, SO EVERY LINE IS ACCOUNTED FOR

**This task is neither of §18a's two shapes.** It is not an ADD (deletions are required — the false comments must die) and not an EXTRACT (nothing moved to a new home). ⇒ **The honest proof is the itemisation, so here are all 18 deletions and all 17 insertions:**

| # | deleted (18) | disposition |
|---|---|---|
| 1-11 | `Charge`'s `// ⚠️ FLAGGED DIVERGENCE…` block, 11 lines | ⛔ **DESTROYED ON PURPOSE — the deliverable.** Not moved, not reworded. |
| 12-13 | `Charge`'s `UE_LOG(…Warning…)` + its `TEXT(…)` line | ⛔ **DESTROYED ON PURPOSE** (false claim + wrong severity) |
| 14 | `Controller->SetUnitCommand(ESiegeUnitCommand::Attack);` | ✅ **REPLACED** by the `ApplyArmyWideStance(Attack)` call |
| 15 | `Fallback`'s `// Same declared divergence as Charge, for the E key.` | ⛔ **DESTROYED ON PURPOSE** |
| 16-17 | `Fallback`'s `UE_LOG(…Warning…)` + its `TEXT(…)` line | ⛔ **DESTROYED ON PURPOSE** |
| 18 | `Controller->SetUnitCommand(ESiegeUnitCommand::Defend);` | ✅ **REPLACED** by the `ApplyArmyWideStance(Defend)` call |

| # | inserted (17) | |
|---|---|---|
| 1-10 | `Charge`'s new 10-line comment | §3(a) — declared addition |
| 11-12 | `Charge`'s `UE_LOG(…Log…)` + `TEXT(…)` | §3(b)/(c) — house idiom, severity corrected |
| 13 | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Attack);` | **the deliverable** |
| 14 | `Fallback`'s 1-line comment | §3(a) |
| 15-16 | `Fallback`'s `UE_LOG(…Log…)` + `TEXT(…)` | §3(b)/(c) |
| 17 | `Controller->ApplyArmyWideStance(ESiegeUnitCommand::Defend);` | **the deliverable** |

⇒ ✅ **Zero lines were lost by accident. 6 of the 18 deletions are the two deliverable swaps and 12 are the deliberate destruction of false text.**

---

## 5. ⛔ DISTURBED NOTHING — 442 / 443 / 455 PROVEN INTACT BY COUNT, NOT BY ASSERTION

**I diffed landmark-symbol counts BEFORE vs AFTER rather than claiming I was careful:**

| symbol | owner | BEFORE | AFTER |
|---|---|---|---|
| `SnapshotTrimBudgetChars` | **TASK-455** | 8 | ✅ **8** |
| `static_assert` | **TASK-455** | 4 | ✅ **4** |
| `EnsureStaticPrefixRegistered` | **TASK-455** | 3 | ✅ **3** |
| `SetStaticPrefix` | **TASK-455** | 5 | ✅ **5** |
| `ExecuteAndReport` | **TASK-443** | 5 | ✅ **5** |
| `IsConfirmBeforeExecuteEnabled` | **TASK-443** | 3 | ✅ **3** |
| `bForceConfirmReview` | **TASK-443** | 6 | ✅ **6** |
| `ClearConfirmPreview` | **TASK-443** | 9 | ✅ **9** |
| `MaxUtteranceBytes` | TASK-433/455 | 4 | ✅ **4** |

- ✅ **`SiegeAssistantComponent.h` NOT TOUCHED** — mtime `13:06:44`, my `.cpp` edit `13:18:16`. **Zero header edits.**
- ✅ **TASK-454's `SiegePlayerController.{h,cpp}` NOT TOUCHED** — I called it, I did not edit it, exactly as pinned.

### ⛔ 443's OFF-PATH STRUCTURE — WHY MY EDIT **STRUCTURALLY CANNOT** MAKE A MACHINE CHECK CONDITIONAL ON THE TOGGLE

**This is the load-bearing one, and it is provable from the call graph rather than by inspection:**

- The toggle is read **once**, in `IsConfirmBeforeExecuteEnabled` (**`:2079`**), from **one** call site — `RouteParsedCommandInternal` (**`:1293`**), its **last** step.
- Both arms converge: the ON arm goes `EnterAwaitConfirm` → `ConfirmPressed` → **`ExecuteAndReport()`** (`:768`); the OFF arm falls through to **`ExecuteAndReport()`** (`:1314`).
- **`ExecutePendingCommand()` has exactly ONE caller: `ExecuteAndReport()` (`:1324`).**

⇒ ⚖️ **`ExecutePendingCommand` is strictly DOWNSTREAM OF BOTH ARMS. My edit is inside it. A change there is by construction reached identically whether the toggle is ON or OFF** — it is in the shared tail 443 built precisely so the two paths *"cannot drift apart under a later edit."* **This edit is that later edit, and the structure held.**

✅ **I added no branch, no early return, no FSM read, no FSM write.** The `Charge`/`Fallback` arms still consist of *log → one call → `return true`*, exactly as before.

---

## 6. ⚠️ THE ONE BEHAVIOUR CHANGE, AND THE ONE THING QA MUST RULE ON

### 6a. `bMatchEnded` NOW GATES THE ASSISTANT'S `charge`/`fallback` — NEW, AND INTENDED

**Verified at the artifact, not relayed:** `ApplyArmyWideStance` (`SiegePlayerController.cpp:1006`) is

```cpp
if (bMatchEnded) { return; }
CancelGroupPick();
ClearAllUnitGroups();
SetUnitCommand(NewCommand);
```

**`grep -rn "MatchEnd" SiegeAssistantComponent.{h,cpp}` → ZERO hits before my edit.** ⇒ **The component had no match-end guard of any kind, so my swap ADDS one to this path.** Post-match, an assistant `charge` previously latched the stance; **it is now inert, exactly like pressing T.** ✅ **That is the consistency the task exists to create** — flagged because it is a change, not because it is wrong.

### 6b. ⛔ THE RULING I NEED: THE EXECUTOR CAN NOW REPORT `Executed` FOR A NO-OP

`ExecutePendingCommand` returns **`true` unconditionally** after the call, and `ApplyArmyWideStance` returns `void`. ⇒ **If `bMatchEnded` is true, nothing happens and `ExecuteAndReport` still pushes `ESiegeAssistantReasonCode::Executed`.**

⚠️ **Reachable window, not theoretical:** the deferred-intent path can fire up to `DeferredIntentTTLSeconds` after the sentence was typed, so the match can end *between* typing and firing.

⚖️ **This is §17's exact shape — "an operation that did not happen, reported in a way nobody reads as failure" — and I am naming it rather than letting it be found.** Three things bound it:

1. **It is not NEW as a class, and it is not confined to my branches.** `ASiegePlayerController::CreateUnitGroup` contains **zero** `bMatchEnded` references, so the assistant's `send`/`guard`/`ambush` **already** form groups post-match and already report `Executed`. **Only `BeginGroupPick` (`:2497`) gates** — and that is the cursor path, not this one.
2. **My change strictly improves the EFFECT** (post-match `charge` stops latching) and leaves only the *reporting* optimistic.
3. **The remedy is available inside my own file and I deliberately did not take it:** `HasMatchEnded()` is **public** at `SiegePlayerController.h:647`, so the component *could* check it and return `false`. ⛔ **I did not, because (i) my spec says "no guard changes", and (ii) fixing it in two branches while `send`/`guard`/`ambush`/`follow` keep the same shape would make the executor LESS uniform, not more.**

📌 **MANAGER/QA: this wants one follow-up task covering ALL executor arms uniformly, not a two-line patch here.**

---

## 7. 📌 STALE SEAM-SPELLINGS I FOUND AND DELIBERATELY DID **NOT** EDIT

**All name the seam as `SetUnitCommand`. None is FALSE** — `ApplyArmyWideStance`'s last statement *is* `SetUnitCommand`, so each is imprecise rather than wrong, which is a different class from the divergence comments I deleted (those asserted a **negative behaviour** that is now false). **All are outside my stated deliverable; two are outside my file entirely.**

| artifact | mine? | note |
|---|---|---|
| `CONVENTIONS.md:823` (§8 executor seam) | ⛔ **manager's file** | ⚠️ **the highest-value one — this is the text a future implementer would "restore" my code to match.** My new comment defends against it in-code, but the doc should be updated at source. |
| `SiegeAssistantCommand.h:59-60` | ⛔ not in my `names:` | *"Charge → SetUnitCommand(Attack)"* |
| `SiegeAssistantSnapshot.cpp:674` | ⛔ not in my `names:` | *"charge / fallback → SetUnitCommand"* |
| `SiegeAssistantComponent.h:182`, `:957` | ✅ my file | seam description; **left alone to keep the diff at the specced shape** |
| `SiegeAssistantComponent.cpp:19` | ✅ my file | include comment listing header APIs; now incomplete (should name `ApplyArmyWideStance`) |

⚖️ **I left all five rather than sweeping them, because a sweep is a different task and would have buried the two-line deliverable QA is meant to see "in one glance."** ✅ **Note `SiegePlayerController.cpp:1021` already says the assistant's `charge`/`fallback` come through this entry point — that was ASPIRATIONAL when TASK-454 wrote it and is TRUE as of this task.**

---

## 8. ⚠️ WHAT CANNOT BE VERIFIED WITHOUT A COMPILE (TASK-447 IS THE GATE)

| claim | status |
|---|---|
| `ApplyArmyWideStance` resolves from this TU | ⛔ **UNVERIFIED BY COMPILER.** Public (`h:474`, inside `public:184-736`), defined (`cpp:1006`), header already included (`:19`). **Low risk — `SetUnitCommand` sat next to it and resolved from the same TU.** |
| `ESiegeUnitCommand::Attack` / `::Defend` still valid here | ⛔ **UNVERIFIED.** Unchanged from the pre-edit code — I altered only the function name, never the argument. |
| the two `Log` lines compile | ⛔ **UNVERIFIED.** No format specifiers, no arguments — plain `TEXT()` literals, the simplest `UE_LOG` shape in the file. |
| the release law actually fires on an assistant `charge` | ⛔ **NOT OBSERVED — requires PIE.** This is a claim about SOURCE. **The runtime proof is TASK-448: type `charge` with a standing group order and watch the group release.** |
| §13 (`TestEqualSensitive`) | ✅ **DOES NOT BIND — I added no test and no `TestEqual`.** Recorded so its absence is not read as an oversight. |

---

## 9. ⚠️ AM I THE LAST THING BEFORE A REVIEWABLE GAME LANE? — ASKED PLAINLY, ANSWERED PLAINLY

**For TASK-446 (the QA gate): YES. It is now fully unblocked.** I checked all ten blockers on the board rather than assuming:

> 440 ✅ · 441 ✅ · 443 ✅ · 444 ✅ · 449 ✅ · 451 ✅ · 453 ✅ · 454 ✅ · 455 ✅ — **all `ready-for-qa`**; **456 becomes the tenth with this handoff.**

⇒ **Nothing stands between here and TASK-446. `SiegeAssistantComponent.{h,cpp}` and `SiegePlayerController.{h,cpp}` are now STILL — no further C++ task is scheduled against either, so 446's citations will not go stale under it.** That was the stated reason the gate was ordered last, and the condition is met.

### ⛔ BUT NOT FOR TASK-447 (THE COMPILE GATE) — ONE C++ TASK REMAINS, AND IT IS NOT IN 446's LIST

**`TASK-457` — [LLM-WARN], `status: backlog`, `gameplay-programmer`, EXCLUSIVE owner of `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp`.** Its own board entry: ***"DISPATCHABLE NOW … PLUGIN module — genuinely parallel-safe against TASK-455 / TASK-456 … ⛔ Must land BEFORE TASK-447, whose smoke test exercises cancel/timeout behaviour."***

⚠️ **So "the last C++ task before TASK-446" is true; "the last C++ task in the wave" is NOT.** 457 is a **different module**, does not block the QA gate, and **can be dispatched right now in parallel with TASK-446** — it touches no file 446 reads. ⇒ 📌 **Dispatching it now costs nothing and keeps it off TASK-447's critical path.**

*(For completeness: `TASK-452` is `backlog` blocked by TASK-421 — an asset dependency, in neither gate's blocker list.)*

---

## 10. WHAT QA SHOULD SCRUTINISE — RANKED

1. ⛔ **RULE ON THE REPLACEMENT TEXT (§3).** I deleted four false artifacts and added 12 lines of true ones against a spec that said *"NOTHING ELSE."* **My mechanism is three citable stale seam-pins that would drive a revert, plus the file's own success-log idiom.** If you rule the literal reading, say so and I will strip it to a bare swap.
2. ⛔ **RULE ON §6b — `Executed` REPORTED FOR A MATCH-ENDED NO-OP.** The highest-consequence item. **I claim it wants a uniform follow-up task across all executor arms, not a patch in my two branches**, and that `HasMatchEnded()` (public, `h:647`) makes it cheap whenever you want it.
3. **CHECK THE DELETION IS TOTAL (§3).** Grep this file for `divergence`, `does NOT run`, `standing group orders survive` — **all three must return nothing.**
4. **CHECK MY STOP-CONDITION ARITHMETIC (§1).** 7 matching lines, 2 call sites. If your denominator differs, mine is wrong — I classified by reading, and ⚠️ **`Grep` mangles `//` on this machine (§14 instance 1), so do any comment-level finding on raw `Read` output.**
5. **CHECK THE OFF-PATH ARGUMENT (§5) STRUCTURALLY, NOT BY INSPECTION.** `ExecutePendingCommand` has exactly one caller (`ExecuteAndReport`, `:1324`) which is the convergence point of both toggle arms. **If that single-caller fact holds, my edit cannot be toggle-conditional.**
6. **THE DIFF INSTRUMENT (§4).** Confirm you accept `git diff --no-index` against a snapshot as the substitute for `git diff --stat` on an **untracked** file — and ⛔ **do not "verify" my change with plain `git diff`, which will show nothing and mean nothing (§14 instance 3).**
</content>
</invoke>
