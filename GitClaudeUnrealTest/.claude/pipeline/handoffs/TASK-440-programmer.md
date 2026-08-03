# TASK-440 — [W1-B1] `CreateUnitGroup` extraction + assistant subobject + mutual-exclusion guards

- **assignee:** gameplay-programmer
- **status on completion:** `ready-for-qa`
- **QA gate:** TASK-446 · **compile gate:** TASK-447 (the ONE gate this batch)
- **files touched (both EXCLUSIVELY mine per ruling 10):**
  - `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
  - `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
- **files NOT touched:** everything else. No `Content/`, no `.ini`, no `Build.cs`, no Git, no editor, no MCP, no PIE, **no compile.**

---

## 0. ⚠️ READ THIS FIRST — THE PRE-FLIGHT CAME BACK **CLEAN**, NOT DIRTY. RULING 7 IS STALE.

The spec, board ruling 7 and CONVENTIONS §9 all say `SiegePlayerController.{h,cpp}` would be **DIRTY** with the FOLLOW lane's uncommitted TASK-395..398 work, and that TASK-447 must split the integration commit **by attribution because pathspec cannot.**

**That is no longer true, and it changes a downstream task's job.** Pre-flight, before I edited anything:

```
$ git status --porcelain -- "*SiegePlayerController*"
(no output — CLEAN)
```

**Jonathan committed the FOLLOW lane himself**, in his own terse style (the standing "Jonathan self-commits milestones" pattern):

```
e70f5ba  follow command, and new miner behavior rework      Sun Aug 2 16:52:59 2026
```

`git show --stat e70f5ba` confirms it carries the FOLLOW work (incl. `Content/Input/Actions/IA_CmdFollow.uasset`, `IMC_Hero.uasset`) alongside the SiegeLlama plugin vendoring.

**Consequences, stated plainly because two of them retire owed work:**

1. ⛔ **I destroyed nothing and reverted nothing.** There was no uncommitted work in these files to edit "additively on top of" — the additive-only instruction was still honoured in substance (I added; I removed no shipped behaviour), but the hazard it guarded against did not exist.
2. ✅ **TASK-447 NO LONGER OWES AN ATTRIBUTION SPLIT ON THESE TWO FILES.** Because they were clean at the moment I started, `git diff` on them right now is **100 % TASK-440 and nothing else.** The two lanes are already separated — by commit, which is stronger than by attribution. **Do not hand-split this commit.** (Section 6 still gives the line-level attribution the spec asked for, so the claim is checkable rather than asserted.)
3. ⚠️ **TASK-402 (Jonathan's FOLLOW/miner playtest gate) IS STILL UNRUN, and flagged item (p) still stands** — my work now sits on top of `e70f5ba` in the working tree, so the next build he playtests covers both lanes at once. That was always the recorded risk; the commit changes who owns the split, not whether the playtest is owed.

⚠️ **I did not "repair" anything and I did not stop.** The spec's stop condition is *"if anything looks wrong"* — a **clean** tree where a dirty one was expected is the **strictly safer** deviation (nothing to lose), so proceeding was the right call, but it is reported here rather than left for QA to discover.

---

## 1. PUBLISHED SIGNATURES — COPY THESE CHARACTER-FOR-CHARACTER

Four downstream tasks compile against this block. **These are now landed in the header; they are not a proposal.**

```cpp
// ── ASiegePlayerController — PUBLIC ───────────────────────────────────────────

/** @return the new group's id, or INDEX_NONE when no member survived the alive-filter. */
int32 CreateUnitGroup(
    ESiegeGroupCommandType Type,
    const FVector& PositionCenter,
    float PositionRadius,
    const FVector& AttackCenter,
    float AttackRadius,
    const TArray<TWeakObjectPtr<ASummonedUnit>>& Members,
    ADecalActor* PositionMarker = nullptr,
    ADecalActor* AttackMarker = nullptr);

/** MOVED private -> public by this task. Body untouched. */
ADecalActor* SpawnGroupCircleDecal(float Radius);

UFUNCTION(BlueprintPure,     Category = "Siegebound|Assistant") USiegeAssistantComponent* GetAssistantComponent() const;
UFUNCTION(BlueprintPure,     Category = "Siegebound|Assistant") bool IsAssistantConsoleOpen() const;
UFUNCTION(BlueprintPure,     Category = "Siegebound|Assistant") bool CanOpenAssistantConsole() const;
UFUNCTION(BlueprintCallable, Category = "Siegebound|Assistant") bool SetAssistantConsoleOpen(bool bOpen);

// ── ASiegePlayerController — PROTECTED ────────────────────────────────────────
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Assistant")
TObjectPtr<USiegeAssistantComponent> AssistantComponent;   // subobject name: TEXT("AssistantComponent")

// ── ASiegePlayerController — PRIVATE ──────────────────────────────────────────
bool bAssistantConsoleOpen = false;
```

**Names match what Wave 0 already wrote down** — `SiegeAssistantCommand.h:56-57` and `SiegeAssistantSnapshot.cpp:672-673` already document the executor seam as `ASiegePlayerController::CreateUnitGroup(Hold, ...)` / `(Ambush, ...)`. No drift.

### What downstream MUST know

| # | Contract | Who it binds |
|---|---|---|
| 1 | ⛔ **`CreateUnitGroup` carries NO `HasAuthority()` guard.** `BeginGroupPick` owns the M8 D5 client lockout for the pick path, so the pick can never reach it on a client. Adding a second, unreachable check would have broken the zero-behavior-change criterion. **TASK-443 must call it only under `HasAuthority()`.** | TASK-443 |
| 2 | **Marker params are OWNERSHIP-TRANSFERRING.** Pass decals and the group owns them (they die with it); **the caller must then null its own refs** or its teardown destroys decals the group owns. Pass nothing for a group that owns no ground. | TASK-443 |
| 3 | ⚠️ **`Members` must not alias into `UnitGroups`** (e.g. a live group's own `Members`). Step 6 appends to `UnitGroups` and can reallocate it. Build a fresh local array. | TASK-443 |
| 4 | **`INDEX_NONE` means "every member was dead"** — an outcome, not an error. The caller owns the player-facing message; `CreateUnitGroup` emits only a log. **A group id IS still consumed on this path** (shipped behaviour, preserved). | TASK-443 |
| 5 | ⛔ **`CanOpenAssistantConsole()` does NOT check `HasAuthority()`.** TASK-442 spec item 7 owns the assistant's authority refusal with the keys' approved wording. Do not assume I covered it. | TASK-442 |
| 6 | **`SetAssistantConsoleOpen(true)` re-gates and can return `false`.** Handle the refusal. `SetAssistantConsoleOpen(false)` never fails. | TASK-442 / TASK-444 |
| 7 | **`SpawnGroupCircleDecal` returns `nullptr` on any degrade** (missing `M_SpellReticle`). Ghost circles must stay null-safe — the confirm step still works with no visual. | TASK-443 |

---

## 2. THE READ-THROUGH ARGUMENT FOR THE EXTRACTION, FUNCTION BY FUNCTION

**Acceptance is "provably zero behavior change" (spec item 1). Here is the proof, statement by statement.**

The shipped stage-3 body ran, in order:

| # | shipped statement | where it lives now | argument |
|---|---|---|---|
| A | `NewGroup.GroupId = NextUnitGroupId++` | `CreateUnitGroup` | Same position — **before** the alive-filter. Deliberately preserved: a refused confirm still burns an id. Moving it below the filter would change every later group's id. |
| B | `Type / PositionCenter / PositionRadius / AttackCenter / AttackRadius / PositionMarkerDecal / AttackMarkerDecal` assignments | `CreateUnitGroup` | Field-for-field identical. The pick passes `GroupPickType`, `GroupPickPositionCenter`, `GroupPickPositionRadius`, `GroupPickLocation`, `GroupPickRadius`, `GroupPickPositionDecal`, `GroupPickActiveDecal` — **the exact members the inline body read**. |
| C | alive re-filter loop | `CreateUnitGroup` | Byte-identical loop, same `Unit && !Unit->IsUnitDead()` predicate, iterating the caller's array instead of `GroupPickSelectedMembers` directly — same object, passed by `const&`. |
| D | empty ⇒ log + `BroadcastRefusal` + `CancelGroupPick` + `return` | **split**: log in `CreateUnitGroup`, `BroadcastRefusal` + `CancelGroupPick` in the caller | **Order preserved exactly**: log fires, then the HUD refusal, then teardown. Log text byte-identical. |
| E | STEAL double loop | `CreateUnitGroup` | Copied verbatim. |
| F | sunflower stations + nav projection + `AssignCommandGroup` | `CreateUnitGroup` | Copied verbatim, including `GoldenAngleRadians`, `StationExtentXY = max(R*0.5, 100)`, `StationProjectExtent(.,.,200)` and the belt-and-braces null `continue`. |
| G | `UnitGroups.Add(MoveTemp(NewGroup))` | `CreateUnitGroup` | Verbatim, with `NewGroupId` captured **before** the move, as shipped. |
| H | `PruneUnitGroups()` | `CreateUnitGroup` | Verbatim. |
| I | `GroupPickPositionDecal = nullptr; GroupPickActiveDecal = nullptr;` | **caller** | Correct home — pick scratch, not group state. |
| J | formation `UE_LOG` | `CreateUnitGroup` | **THE ONE REORDER — see §3.** Text byte-identical incl. the `(TASK-344)` tag; operands translated `GroupPickPositionCenter→PositionCenter`, `GroupPickLocation→AttackCenter`, etc. — same values. |
| K | `CancelGroupPick()` | caller | Unchanged. |
| L | completion `BroadcastCommandPrompt("%s set: %d unit(s)")` | caller | Unchanged text; see §4 for how `MemberCount` is now obtained. |

**Aliasing check (a hazard the extraction introduces and the inline body could not have):** `PositionCenter` / `AttackCenter` / `Members` are `const&` **to live controller members**. They must stay valid and unmutated for the whole call. They do: the only things that run between binding and last use are the steal loop and `PruneUnitGroups()` (both touch `UnitGroups` / `DefaultFollowGroupId` only — verified by reading `PruneUnitGroups`), and `ASummonedUnit::AssignCommandGroup`, which I read (`SummonedUnit.cpp:1886-1906`) and which is **four scalar writes on the unit with no callback into the controller**. `CancelGroupPick` — the one function that *does* reset `GroupPick*` — is called by the caller **after** `CreateUnitGroup` returns.

---

## 3. ⚠️ THE ONE REORDER, STATED PLAINLY RATHER THAN BURIED

**The formation `UE_LOG` moved from AFTER the two marker-ref nulls (I) to BEFORE them.**

It is inert, and here is the whole proof: the log's operands are `GetNameSafe(this)`, `TypeLabel`, `NewGroupId`, `MemberCount` and the four zone scalars — **it reads neither `GroupPickPositionDecal` nor `GroupPickActiveDecal`**, which are the only two things statement I writes; and `UE_LOG` cannot re-enter gameplay code. Swapping two statements with no data dependency and no side-effect overlap changes nothing observable.

I considered the two alternatives and rejected both, for the record:
- **Null the refs before the call** (capturing decals into locals first): also a reorder, but a *worse* proof — it would move statement I across `AssignCommandGroup`, a call into another class, so the argument would depend on that method's callback surface instead of on `UE_LOG`'s.
- **Leave the log in the caller:** zero reorder, but then the extracted function has no formation log and TASK-443 either duplicates it or ships silently. One implementation means one log.

**This is the only ordering change in the task.** If QA disagrees with the trade, the fix is mechanical (move the log back to the caller) and I will take it without argument.

---

## 4. THE ONE VALUE THAT IS NOW DERIVED RATHER THAN CARRIED

The completion prompt needs `MemberCount` — the count that **joined**, after the alive-filter, which is not `GroupPickSelectedMembers.Num()`. The inline body had it as a local; the caller now reads it back:

```cpp
int32 MemberCount = 0;
if (const FSiegeUnitGroup* FormedGroup = FindUnitGroup(NewGroupId)) { MemberCount = FormedGroup->Members.Num(); }
else { /* UE_LOG Warning — see below */ }
```

**Provably the same number:** the group was just appended with ≥1 member; `PruneUnitGroups` cannot reap a group that still has members, and cannot drop a member that passed the identical `!IsUnitDead()` test one statement earlier with no tick in between.

I added a `Warning` on the unreachable `else` rather than assuming — so a future change to `PruneUnitGroups` cannot silently make the HUD prompt lie. Degradation is "reports 0 unit(s)", never a crash.

⚠️ **This is the one place a reviewer should push if they want to push somewhere.** It is a lookup where the shipped code had a local. It is exact today; it is exact by an argument rather than by construction.

---

## 5. THE OTHER FOUR DELIVERABLES

**(2) The `USiegeAssistantComponent` default subobject** — created in the constructor beside `DeckComponent`, subobject name `TEXT("AssistantComponent")`. ⛔ I created it and authored **nothing** inside it; the class body is TASK-442's.

**(3) ONE `ApplyCursorInputState` term** — `bAssistantConsoleOpen` joins the existing OR:
```cpp
const bool bWantCursor = bInPlacementMode || bInTargetingMode || (GroupPickStage != EGroupPickStage::None) || bAssistantConsoleOpen || bUICursorHeld;
```
**No parallel path, no second `SetInputMode` call site.** The flag defaults `false`, so every pre-existing flow evaluates identically. `SetAssistantConsoleOpen` is the only writer, and it re-applies the posture on every real change.

**(4) FOUR MUTUAL-EXCLUSION GUARDS, SYMMETRIC:**

| direction | site | behaviour |
|---|---|---|
| console blocks placement | `EnterPlacementMode` | new `if (bAssistantConsoleOpen)` Verbose ignore, appended **AFTER** the shipped three so their precedence and log lines are untouched |
| console blocks targeting | `EnterTargetingMode` | same shape |
| console blocks group pick | `BeginGroupPick` | same shape (covers R / F / **C**) |
| the other three block the console | `CanOpenAssistantConsole()` | `!bMatchEnded && !bInPlacementMode && !bInTargetingMode && GroupPickStage == None`, re-gated inside `SetAssistantConsoleOpen` so it cannot be bypassed |

⚠️ **DELIBERATE ADDITION FOR QA TO RULE ON: `CanOpenAssistantConsole` also refuses after match end.** That is a **fifth** condition, not one of the four. Grounds: it is the shipped `Enter*` / `BeginGroupPick` idiom (all three check `bMatchEnded` first), and it is load-bearing rather than cosmetic — **`ApplyCursorInputState` early-outs while `bMatchEnded` is latched**, so a console opened on the end screen would never receive its cursor posture. It refuses *more*, which is the fail-safe direction. **If QA rules it out of scope, deleting the clause is a one-line change.**

⚠️ **CONVENTIONS §2 (strictly additive) survives:** the keys are refused **only while the console is open**, i.e. only while the player is typing into it. Console closed ⇒ `IA_CmdFollow` / `CmdAttack` / `CmdHold` / `CmdDefend` / `CmdAmbush` / `IA_Rally` / cards 1–6 hit **byte-identical code paths** — every new branch is `if (bAssistantConsoleOpen)` on a flag that is `false`.

**(5) `SpawnGroupCircleDecal` widened `private` → `public`.** **Body untouched — access change only.** A `friend class USiegeAssistantComponent` was considered and **rejected**: friendship exposes *every* private member of the controller to reach one function, which is the wider grant. Public-on-one-function is the narrower one. The old declaration site carries a `//~` breadcrumb so the move is discoverable.

---

## 6. LINE ATTRIBUTION (what the spec asked for; §0 explains why TASK-447 no longer needs it)

`git diff --stat` on my two files, which were **clean** when I started:

```
 .../Siegebound/SiegePlayerController.cpp   | 347 ++++++++++++++++-----
 .../Siegebound/SiegePlayerController.h     | 172 +++++++++-
 2 files changed, 425 insertions(+), 94 deletions(-)
```

⇒ **Every one of those 425/94 lines is TASK-440.** The FOLLOW lane's lines are in `e70f5ba`, already committed, and appear in **zero** of them. Change sites:

- **`.h`** — fwd-decl `USiegeAssistantComponent` · `CreateUnitGroup` decl + doc · `SpawnGroupCircleDecal` moved to public (+ breadcrumb at the old site) · the 4-entry `Siegebound|Assistant` block · `AssistantComponent` UPROPERTY · `bAssistantConsoleOpen` · doc amendments on `ConfirmGroupPickStage` + `ApplyCursorInputState`.
- **`.cpp`** — `#include "Siegebound/SiegeAssistantComponent.h"` · ctor subobject · `EnterPlacementMode` guard · `EnterTargetingMode` guard · `BeginGroupPick` guard · stage-3 case rewritten as a call · **new** `CreateUnitGroup` · `ApplyCursorInputState` term · **new** `CanOpenAssistantConsole` + `SetAssistantConsoleOpen`.

---

## 7. ⛔ WHAT I COULD NOT VERIFY — I DID NOT COMPILE, AND NOTHING BELOW BUILT

**No compiler, no UHT, no linker, and no engine has seen a single line of this.** The one compile gate is build-master's TASK-447 under the quiet-module law. Concretely un-checked:

1. ⛔ **`#include "Siegebound/SiegeAssistantComponent.h"` REFERENCES A FILE THAT DOES NOT EXIST YET.** TASK-442 creates it. **This file cannot compile until TASK-442 lands** — that is the expected and specified state (§8 pin: *"several of these cannot compile alone and are not expected to"*; the TASK-416/417 precedent). ⛔ **Do not open a QA loop over it.** If TASK-442's header lands at a different path or names the class differently, **this include is the first thing that breaks.**
2. **UHT has not run.** The four new `UFUNCTION`s, the new `UPROPERTY` on a forward-declared `USiegeAssistantComponent`, and `GetAssistantComponent()` returning that forward-declared type are all *believed* legal (standard UE) but are **unparsed**.
3. **No overload/ambiguity check** on `CreateUnitGroup`, and no verification that `TObjectPtr<ADecalActor> → ADecalActor*` implicit conversion at the two call-site arguments compiles as expected.
4. **Zero runtime verification.** No PIE, no editor, no MCP — Jonathan is at the keyboard with the model loaded. **Nothing here has executed.** "Reviewed by reading" is legitimate and **incomplete**, and the parser I did not run is the one that decides.
5. **The behavioural claim is a READ-THROUGH claim.** §2's table is an argument from source, not an observation of a running game. **Nobody has pressed R, F or C on this build.**

### What must be checked at TASK-447 / TASK-448
- **TASK-447 (compile):** the include in (1) resolves; UHT accepts the new reflected members.
- **TASK-448 (Jonathan, playtest):** `R` (HOLD) and `F` (AMBUSH) still form groups with correct **member counts**, **ground markers** and **HUD prompts** — that is the extraction's live proof. `C` (FOLLOW) unchanged. `T` / `E` release unchanged. **This is also the FOLLOW/miner playtest that TASK-402 still owes (flag (p)) — both lanes are in this build.**

---

## 8. NOT DONE — DELIBERATELY, AND WHY

- ⛔ **No component body, no console widget, no executor, no settings read.** TASK-442 / 443 / 444.
- ⛔ **No `PlayerTick` change.** Not needed and therefore not made: the pick/placement poll branches only run while their mode is live, and the console cannot open while any of them is — so the exclusion is self-consistent without touching the tick.
- ⛔ **`EnsureDefaultFollowGroup` was NOT routed through `CreateUnitGroup`,** and this is a correctness point rather than an omission: it creates an **empty, zero-radius, marker-less** follow group, and `CreateUnitGroup` **refuses on zero members by design** (shipped stage-3 behaviour). It structurally cannot use it. It is also the FOLLOW lane's code, which I do not touch.
- ⛔ **No cleanup, reformat or "tidy" anywhere in either file.**

---

## M8 DECLARATION (verbatim, as required)

**"adds no replicated property, no new replicated class, no new relevancy tier."**

**And why it is true here rather than only asserted:** `bAssistantConsoleOpen` is a plain (non-`UPROPERTY`) bool describing **local UI focus**; `AssistantComponent` is a default subobject on a controller whose `bReplicates` flag and replicated surface are **unchanged by this task**; and `CreateUnitGroup` writes `UnitGroups`, which is **already** the shipped, non-replicated, host-side group store (`BeginGroupPick`'s M8 D5 lockout keeps clients out of it entirely — P2 owns the server-side migration). **No `GetLifetimeReplicatedProps` entry was added, changed, or needed.**

## Compliance

No Git · no compile · no editor / MCP / PIE · sealed holdout untouched · `L_Arena` untouched · stayed inside my two files.
