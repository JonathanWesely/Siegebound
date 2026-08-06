# TASK-548 — THE EXECUTOR: the region predicate, the loud refusal, `DescribeCommandForPlayer`

- **task:** TASK-548 (AC-8) · **assignee:** gameplay-programmer · **status:** `ready-for-qa`
- **gate:** ⛔ **TASK-550** (`qa/TASK-550.md` — the batch's only gate)
- **files touched (exactly two, both in scope):**
  - `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`
  - `Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h`
- ⛔ **NOT touched:** `SiegeAssistantCommand.*` (545) · `SiegeAssistantGrammar.*` (546) · `SiegeAssistantSnapshot.*` (547) · `SiegeAssistantRegionStatics.*` (544) · any `Tests/` file (549) · `SiegePlayerController.*` · `HeroCharacter.*` · `AncientGround.*` · `CaptureZone.*` · any `Build.cs` · any `Content/` asset · any CSV.
- ⛔ **No compile** (TASK-551 owns the only compile). ⛔ **No Git.** ⛔ **No tests** (TASK-549).

📌 **M8, verbatim:** *"adds no replicated property, no new replicated class, no new relevancy tier."* The reason is structural: `RegionPlace` is an `FName` on a struct already `uint8`/`int32`/`FName`-only, `FSiegeAssistantRegionStatics` is a non-UObject static library, and the membership test runs where the selector already runs — **on the authority**, inside `SelectUnitsForOrder`, which is only reached from `ExecutePendingCommand` after its `HasAuthority()` gate.

---

## 1. ⭐⭐ THE RECEIVE-SIDE GATE NOW PASSES `Command.RegionPlace` — THE ONE-GREP PROOF

**IT IS DONE.** `SiegeAssistantComponent.cpp` **:1051** (was `:1039` before TASK-542's log line pushed it down — I located it **by symbol**, not by offset):

```cpp
// BEFORE (validated NOTHING about the region, and compiled clean):
if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError, Command.ExcludeKinds))

// AFTER:
if (!SiegeAssistantValidateSelection(Command.Kinds, Command.Counts, SelectionError, Command.ExcludeKinds, Command.RegionPlace))
```

**THE GREP THAT PROVES IT IN ONE LOOK** — run it yourself, do not take my word:

```
grep -rn "SiegeAssistantValidateSelection(" Source/ Plugins/ --include=*.cpp --include=*.h
```

**Result (16 hits, and every one is accounted for):**

| hit | verdict |
|---|---|
| `SiegeAssistantCommand.h:616` / `SiegeAssistantCommand.cpp:449` | the **declaration** and the **definition**. Not call sites. |
| `SiegeAssistantCommand.cpp:1095` | ✅ whole-command caller (the parser's own) — passes **`Parsed.ExcludeKinds, Parsed.RegionPlace`**. TASK-545's, already correct. |
| **`SiegeAssistantComponent.cpp:1051`** | ✅ **the receive-side gate — MINE, now passes `Command.ExcludeKinds, Command.RegionPlace`.** |
| 12 hits in `Tests/SiegeAssistantGrammarTest.cpp` + `Tests/SiegeAssistantSelectionTest.cpp` | ✅ **arrays-only** — none holds an `FSiegeAssistantCommand`, so the trailing defaults are being used exactly as intended (and two of them exist specifically to assert the 3-arg/4-arg forms). |

⇒ ⭐ **There are exactly TWO whole-command call sites in the module and BOTH pass all five arguments.** `AS-§21.9`'s QA criterion is met.

The comment block above the call was rewritten to name **both** the fourth and the fifth argument, and it now **records the gap rather than quietly closing it**: between TASK-545 landing the parameter and this edit, the last machine check standing between a well-formed model emission and the executor validated nothing at all about the region, and it compiled, linked and reported SUCCESS the whole time.

---

## 2. ⭐⭐ A SECOND TRAILING-DEFAULT HAZARD — **FOUND BY ME, IN MY OWN FILE, NAMED BY NO TASK, AND IT WOULD HAVE KILLED THE ENTIRE BATCH SILENTLY**

⚠️ **QA: this is the finding I most want a second pair of eyes on.**

`SiegeAssistantComponent.cpp:3399` (`ComposeTurnGrammar`) is **the module's ONLY shipped (non-test) caller of `USiegeAssistantGrammar::Build`**, and it was passing **two** arguments to a function that TASK-546 gave a **third, trailing, defaulted** one:

```cpp
// BEFORE — compiled, linked, ran, and produced a grammar with NO `inplace` rule:
return USiegeAssistantGrammar::Build(Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames());

// AFTER:
return USiegeAssistantGrammar::Build(Snapshot->GetUnitKinds(), Snapshot->GetPlaceNames(), Snapshot->GetRegionPlaceNames());
```

**WHY IT IS THE BATCH-KILLER AND NOT A NICETY.** Omitting the argument yields a grammar with **no `inplace` rule, no `zone` alternation and no `in` alternative on `who`.** Constrained decoding **cannot sample a shape the grammar does not contain** ⇒ `{"in":"ancient_ground_near"}` would have been **structurally unreachable at runtime**, and TASK-544's statics, TASK-545's parser + struct, TASK-546's grammar, TASK-547's snapshot and everything I wrote today would have sat there **correct and unreached**. Jonathan's sentence B would have failed at Stage 5 exactly as it fails today.

⚠️ **AND THE FAILURE WOULD HAVE BEEN MISDIAGNOSED, WHICH IS THE WORSE HALF.** `AS-§21.11` outcome 5 already primes every reader to expect *"a rule line is a WEAKER teaching signal than an exemplar for a brand-new output SHAPE"* — so **"the model never emitted `in`"** at Stage 5 would have been read as **the prompt under-teaching the shape**: a conclusion about the *model*, drawn from a *missing function argument*, with a few-shot spend and a sealed-holdout argument queued up behind it.

**PROVENANCE — it is not a freelance edit:**
- `SiegeAssistantGrammar.h:148-149` (TASK-546, shipped): *"⇒ **ANY CALLER HOLDING A SNAPSHOT PASSES `Snapshot->GetRegionPlaceNames()`.** The pin is `AS-§21.9`."*
- `handoffs/TASK-547-programmer.md` §10 (DOWNSTREAM CONTRACT, TASK-548): *"`Snapshot->GetRegionPlaceNames()` → the grammar's `RegionPlaceNames` parameter (TASK-546's third defaulted arg)."*
- The line lives in `SiegeAssistantComponent.cpp` — **my file, and only my file.** TASK-546 owns the builder and **could not reach this call site**; no other task in 541..551 owns it. **If I had not taken it, nobody would have.**

**PROVING GREP:** `grep -rn "USiegeAssistantGrammar::Build(" Source/ --include=*.cpp | grep -v "/Tests/"` → **exactly one call site, and it now passes three arguments.** (The test call sites pass two, deliberately — TASK-549 owns whether any of them should grow a third.)

✅ **An empty region list is a legal, handled state and needs no guard here:** a map with no ancient ground and no capture zone publishes no regions, and TASK-546's builder then omits `zone`/`inplace` and the `who` alternative **entirely** — which is *required*, because an empty alternation would leave `zone` undefined and make the whole grammar unparseable (`AS-§21.4`, the `at_least` disaster). The snapshot decides; this line only relays.

---

## 3. THE SELECTOR — the diff, in execution order

All five pieces are in `SelectUnitsForOrder`. ⛔ **Still ONE pass over the world. No registry, no actor cache, no dirty flag, no subscription list** (`AS-§4` rejects all four on sight). The addition is *"a **PREDICATE INSIDE A LOOP THAT ALREADY EXISTS**"* — two float compares per candidate and **not one new traversal**.

### (a) `:2057` — THE BACKSTOP (Warning, refuses the whole order)

```cpp
if (Command.RegionPlace != NAME_None && (Command.Kinds.Num() > 0 || Command.ExcludeKinds.Num() > 0))
```

Mirrors the shipped exclusion backstop immediately above it, at **Warning** for the same stated reason (the refusals further down are the *model* being wrong — expected traffic at `Log`; reaching **this** line is a **code or wire defect**, a struct that passed neither gate). Unreachable today; *"unreachable" is a claim about today's callers.*

⚠️ **THE ASYMMETRY WITH THE EXCLUSION BACKSTOP IS REAL AND IS WRITTEN INTO THE COMMENT** — same verdict, different reason. An exclusion beside a selection would be **dropped in silence** (the per-kind loop has no subtraction step). A region beside either would **NOT** be dropped: my predicate sits in the shared candidate gather, so it would apply and the two filters would **compose**. `AS-§21.5` rules that combination *"a parse FAILURE, never a merge"* — *"send 10 footmen in the mid"* is a **filtered count**, a different feature nobody asked for. ⛔ Guessing which half to honour is the valid-shaped-wrong-command class; **so is silently honouring both.**

### ⚠️ (b) DECLARED WIDENING — the backstop also covers `RegionPlace` + `ExcludeKinds`, which the dispatch did not literally name

**Flagged, not slipped in. QA may rule it back.**
- The dispatch says *"a command reaching the selector with a region AND a **positive selection**."*
- **`AS-§21.5` says in terms: *"`who:"none"` plus a region is the same failure. `all_except` plus a region is the same failure."*** The parser refuses both with `region_conflict`; `SiegeAssistantValidateSelection` refuses both.
- Refusing only one leg would leave the other **composing quietly** — the exact defect the backstop exists to catch.
- Both legs are unreachable from the model and from the M8 P2 wire path, for the same reason the exclusion backstop is: two gates already refuse them. **The widening costs nothing behaviourally and fails closed.**

### (c) `:2104-2113` — HARD RULE 1: A REGION NAMED AND NOT RESOLVED IS A **REFUSAL**

```cpp
FVector RegionCentre = FVector::ZeroVector;
FVector2D RegionHalfExtent = FVector2D::ZeroVector;
const bool bHasRegion = Command.RegionPlace != NAME_None;
if (bHasRegion && (!Snapshot || !Snapshot->ResolvePlaceRegion(Command.RegionPlace, RegionCentre, RegionHalfExtent)))
{
    UE_LOG(LogSiegeAssistant, Log, TEXT("Selector refused - region '%s' did not resolve at execution time …"), …);
    return false;
}
```

- ⛔ **It does NOT fall through to "everybody."** Falling through would execute *"send everyone in the ancient ground"* as *"send everyone"* — an army moving that the player never asked to move, the `AS-§20.1` class this whole design exists to prevent. **Fail closed.**
- ⛔ **No "whole map" fallback box, no zero-extent default.** `ResolvePlaceRegion` leaves both out-params untouched on failure for precisely this reason; inventing a box here would re-open the hole from the other side. The two locals are initialised only so a reader can see they are never read on the failure path.
- ⛔ **Resolved ONCE, above the loop.** The geometry is snapshot-time and constant for the call; only the positions are live (`AS-§21.4` — *the units move, the grounds do not*).
- ⚠️ **A missing `Snapshot` takes the same refusal** (short-circuit `||`), so there is no path where `bHasRegion` is true and the box is unread.
- ⚠️ **`Log`, not `Warning`** — matching `ExecuteZoneOrder`'s sibling *"place did not resolve at execution time"* line. This is a **board condition** (the snapshot's own declared residual: a ground destroyed between `Capture()` and execution lands exactly here), not a code defect, and the automation runner reads `Warning` as failure.

### (d) `:2190-2194` — THE PREDICATE, in the candidate loop that already exists

```cpp
if (bHasRegion && !FSiegeAssistantRegionStatics::IsPointInRegion(Unit->GetActorLocation(), RegionCentre, RegionHalfExtent))
{
    ++OutsideRegionCount;
    continue;
}
```

Immediately **after** the eligibility gate, immediately **before** the candidate is built.

- ⛔ **A pure call to TASK-544's shipped predicate.** ⛔ No inline box test. ⛔ No direct `AAncientGround::IsPointInZone` — the executor holds **no actor**; the snapshot holds the geometry, deliberately.
- ⭐ **IT SITS IN THE SHARED CANDIDATE GATHER, NOT IN THE `Kinds.Num() == 0` BRANCH — and that placement is the whole reason it applies to BOTH selector branches from ONE piece of code.** Both branches consume `Eligible`, so there is no duplicated predicate and no branch that "never learned about" the region.
- ⚠️ **This is a DECLARED DEPARTURE from the board's spec item (1)**, which says *"in the `Command.Kinds.Num() == 0` branch where the exclusion filter already lives."* The dispatch overrode it (*"it must apply to BOTH selector branches, unlike `ExcludeKinds` which is `"all"`-only — 'the footmen in the ancient ground' is as sensible as 'everyone in the ancient ground'"*), and the dispatch's own placement instruction (*"the candidate-gathering loop that already exists, immediately after the eligibility gate"*) is what makes both-branch coverage possible without a second copy. **I followed the dispatch.** The board text and the dispatch text disagree; the dispatch is the later instruction and the better code.
- ⚠️ **The per-kind branch is UNREACHABLE with a region today** — the parser refuses region+selection and my own backstop refuses it again — so the predicate's effect on that branch is currently **unobservable**. Written correctly anyway: if a later ruling opens the combination, the filter is already there. A region parsed and then dropped by a branch that never learned about it is the exact failure `AS-§21.6` names.
- ⚠️ **`GetActorLocation()` is called twice per candidate** (once here, once in the untouched `DistSq` line). Deliberate: hoisting it into a local would have edited the sort line, and **a purely additive diff is this batch's main safety argument.** It is an inlined transform read, not a query.
- ⛔ **SORTING IS UNTOUCHED** (spec item 7). `DistSq` computes exactly what it always computed, against the same anchor; the comparator is byte-identical. **The region FILTERS; it does not re-rank.**

### (e) `:2232-2239` — HARD RULE 2: EMPTY-AFTER-REGION IS A **LOUD REFUSAL WITH THE ARITHMETIC**

```cpp
if (bHasRegion && Eligible.Num() == 0)  →  UE_LOG(Log, "…%d eligible…, %d standing OUTSIDE region '%s', %d left…")  →  return false;
```

- ✅ Routed through the **EXISTING unsupported-ask outcome** — `return false` → `ExecuteZoneOrder`/`ExecuteFollowOrder` return false → `ExecutePendingCommand` returns false → `ExecuteAndReport` pushes **`ESiegeAssistantReasonCode::AskUnsupported`**. ⛔ **No new ask symbol. No new reason code. No new template row. No silent no-op.**
- ⭐ **CHECKED BEFORE THE SORT AND BEFORE EITHER BRANCH, PRECISELY SO NEITHER BRANCH CAN MISREPORT IT.** Without it, an empty candidate set falls into the shipped *"Selector found no eligible unit at all for an army-wide selection"* line (or, on the per-kind side, *"no ELIGIBLE '<kind>' is alive on the ordering team"*) — **both TRUE-SOUNDING AND WRONG** when the units exist and are simply standing somewhere else. A refusal that names the wrong cause sends the reader to the roster instead of to the ground.
- Mirrors the empty-after-exclusion line **for line**, including the *"nothing happened and nothing was said"* clause and the `NOTHING was executed.` terminator.

### (f) `:2241-2257` — THE SUCCESS LOG (spec item: *"a log that only speaks on failure cannot prove a success"*)

`Selector applied a REGION: %d eligible, %d outside region '%s', %d INSIDE and carried forward as candidates.` — the exclusion's success line has a twin, for the identical stated reason: the region filter **cannot be reached by the automation suite** (EditorContext simple tests have no world and no actors), so this line **plus Jonathan's TASK-552 playtest** is the only evidence the region actually filtered rather than being parsed and dropped.

⚠️ It says **"candidates"**, not "selected": the per-kind take and the exclusion both run downstream. Today they cannot coexist with a region, so the numbers coincide — but the word stays true of the *code*, not of today's callers.

### ⭐ (g) THE REFUSAL ARITHMETIC DOES **NOT** MISREPORT `AS-§21.11` OUTCOME 2

Ogres, Sappers and Clerics can never take a zone order at all (`CanTakeZoneOrders()` is Standard-profile only), so a region order **correctly** skips them — a **designed outcome, not a bug**. Three things keep the numbers honest:

1. **"Eligible" is counted AFTER the eligibility gate.** A skipped Ogre is **never** counted as *"outside the region"* — the region test never runs on it. Printed eligible = `OutsideRegionCount + Eligible.Num()`, derived from the two survivors' counters rather than a third counter that could drift.
2. **`IneligibleCount` is a separate figure** (incremented at the existing eligibility `continue`, zero extra passes) and both log lines **name the predicate that actually ran** — `IsGroupCommandEligible()` vs `IsFollowCommandEligible()` — because the answer genuinely differs between a zone order and a follow (Clerics **can** follow).
3. ⛔ **BOTH LINES STATE WHAT THEY DID NOT MEASURE:** `IneligibleCount` is a **whole-team** figure, **not** "ineligible units inside the region", so the log says in terms that it *"cannot tell you whether the region is EMPTY or merely holds units that could never take this order."* Measuring the latter would mean region-testing units the eligibility gate already dropped — a claim I refuse to imply from a number I did not compute.

⇒ *"Everyone in the ancient ground"* over a ground holding three Ogres logs **`0 eligible, 0 outside, 0 left, a further 3 never reached the region test — IsGroupCommandEligible() rejected them first (AS-§21.11 designed outcome 2, NOT a bug)"** — which is exactly the sentence that stops this being filed as a region-filter defect.

---

## 4. ⭐ `DescribeCommandForPlayer` — `:1346-1364`

```cpp
if (Command.RegionPlace != NAME_None)
{
    if (Selection.IsEmpty()) { Selection += FString::Printf(TEXT("all in %s"), *Command.RegionPlace.ToString()); }
    else                     { Selection += FString::Printf(TEXT(", in %s"),  *Command.RegionPlace.ToString()); }
}
```

**RENDERING — read aloud on every path its values can supply** (the TASK-471 mechanical test):

| command | before | **after** |
|---|---|---|
| `send` + `{"in":"ancient_ground_near"}` + `where:enemy_castle` | ⛔ `Send (enemy_castle)` | ✅ **`Send all in ancient_ground_near (enemy_castle)`** |
| `guard` + `{"in":"mid"}` + `where:own_castle` | ⛔ `Guard (own_castle)` | ✅ **`Guard all in mid (own_castle)`** |
| `follow` + `{"in":"ancient_ground_far"}` (no place) | ⛔ `Follow` | ✅ **`Follow all in ancient_ground_far`** |
| region + selection (unreachable) | — | `Send 8 footman, in mid (enemy_castle)` |
| region + exclusion (unreachable) | — | `Send all except miner, in mid (enemy_castle)` |
| **no region** | unchanged | **byte-identical — the clause is skipped entirely** |

- ⛔ **NO NEW TEMPLATE ROW AND NO NEW FRAME.** The clause is built into **`{Selection}`**, exactly as the exception is, so **all three existing frames** (`SelectionPlace` / `Selection` / `Place`) render it unchanged and `§30`'s *"one string, not three"* stays true. It follows the shipped `all except ` idiom character-for-character: a **raw symbol**, never a display-name lookup.
- ⚖️ **THE TASK-522 ARGUMENT, VERBATIM AND FOR THE SECOND TIME:** a confirm prompt that describes a DIFFERENT order from the one that will execute **defeats the confirm step's entire purpose.** Before this, a region order rendered as a **whole-army** sentence — `Send (enemy_castle)` — and the player would have accepted the wrong order. The same string is reused by the `Executed` line and the deferred *"waiting for…"* line.
- ⛔ **THE PRESENTATION LAYER DOES NOT REPAIR ITS INPUT (`SC-§31`).** It describes what **will run**, including a region that will resolve to nobody; the refusal and its arithmetic belong to the selector. ⛔ And it never runs the selector to count occupants — that would be a second, staler survey of a board that can change before the player presses accept.
- ⚠️ **The unreachable `, in %s` branch is written anyway**, on the identical reasoning as the exception's twin branch: a describer that drops a filter it was handed prints a reassuring sentence about an order nobody gave, and this function is called from paths (`LastMessage` re-seeding, `GetPendingConfirmSummary`) that do **not** re-run the gates.

### ⚠️ THE GHOST-CIRCLE PREVIEW STRUCTURALLY CANNOT REFLECT THIS — NOT ATTEMPTED, AND NOTED

TASK-522's finding applies unchanged: `SpawnConfirmPreview` draws **two PLACE decals at the resolved destination and nothing per-unit** — identical geometry for 3 units or 30 — so **no preview shape can depend on WHICH units were selected.** A selection filter is invisible on the ground either way. ⇒ **`DescribeCommandForPlayer`'s sentence IS the unit-facing half of the review.** I did not try to make the preview carry it and no code was written toward it.

---

## 5. ⭐ ANSWER TO THE QUESTION TASK-549 MUST NOT GUESS: **YES, `DescribeCommandForPlayer` IS HEADLESSLY REACHABLE**

**Verified by reading, not assumed:**
- It is **`public`** (`SiegeAssistantComponent.h:639 public:` … `:1075 private:` — the declaration is inside that block), a plain **`const` member function**, **not** a `UFUNCTION`, signature `FText DescribeCommandForPlayer(const FSiegeAssistantCommand&) const`.
- **Its body touches NO member state, NO `UWorld`, NO `Snapshot`, NO subsystem, NO actor.** It reads only the `Command` parameter and calls `SiegeAssistantComponentInternal::IntentDisplayText`, which is a file-local `static` switch over `static const FText` literals (`SiegeAssistantComponent.cpp:142-155`) — no world either.
- ⇒ **`NewObject<USiegeAssistantComponent>()` on the transient package is enough. No world, no actor, no `RegisterComponent`, no `BeginPlay`.**

📌 **TASK-549: assert the `all in <place>` rendering.** ⚠️ Two practical notes: **hold the component in a `TStrongObjectPtr`** (or an `FGCObjectScopeGuard`) for the test's scope so a GC pass mid-test cannot collect it; and use **`TestEqualSensitive`** on the rendered string (`SC-§13`) — `FText::ToString()` gives you the bytes. ⛔ **Do not fake a world.**

---

## 6. ⚠️ WHAT QA SHOULD SCRUTINISE — my own list, hardest first

1. ⭐⭐ **RE-RUN THE TWO GREPS IN §1 AND §2 YOURSELF.** They are the whole proof for both trailing-default hazards, and **no compiler diagnostic stands behind either.**
2. ⭐⭐ **THE §2 FINDING — is the third `Build` argument correct, and is it in scope?** I am confident on both counts (TASK-546's header pins it, TASK-547's handoff assigns it to me, the line is in my file and nobody else's), but it was **named by no task**, and a wrongly-shaped fix here silently disables the feature in the *other* direction. **Check `Snapshot->GetRegionPlaceNames()` returns `const TArray<FName>&` and binds to the parameter** (it does — `SiegeAssistantSnapshot.h:599`).
3. ⚠️ **THE DECLARED WIDENING in §3(b)** — the backstop covers region+exclusion as well as region+selection. **Rule it, do not leave it ambiguous.** My case is `AS-§21.5`'s own sentence; the cost of being wrong is a `Warning` on an unreachable path.
4. ⚠️ **THE PLACEMENT DEPARTURE in §3(d)** — board spec item (1) says the `Kinds.Num()==0` branch; the dispatch says the shared candidate loop and demands both branches. **These cannot both be satisfied by one predicate in the "all" branch.** I followed the dispatch. If QA prefers the board text, the both-branch requirement has to be dropped with it.
5. **THE ARITHMETIC, §3(g).** Check that *eligible* = `OutsideRegionCount + Eligible.Num()` really cannot count a profile-ineligible unit, and that neither log line implies a region fact about `IneligibleCount`. **This is the line most likely to turn a designed outcome into a false bug report on Jonathan's sheet.**
6. **THE REFUSAL ORDER.** Backstop (Warning) → world → team → region resolve (Log, fail-closed) → gather+filter → empty-after-region (Log) → success log → sort → branches. **Confirm no path reaches a branch with `bHasRegion` true and an empty `Eligible`.**
7. **VERBOSITY.** Backstop = `Warning` (code defect). Unresolved region + empty-after-region + success = `Log` (board conditions / the model being wrong). ⛔ Nothing new at `Warning` on a path the model can drive; ⛔ nothing per-tick — `SelectUnitsForOrder` runs **once per executed order**.
8. **`DescribeCommandForPlayer` with no region is byte-identical.** The clause is inside `if (Command.RegionPlace != NAME_None)`; every shipped rendering is untouched.

---

## 7. STANDING TRAPS — checked, one by one

| trap | status |
|---|---|
| shadowing an inherited reflected member | ✅ new locals are `RegionCentre`, `RegionHalfExtent`, `bHasRegion`, `OutsideRegionCount`, `IneligibleCount` — none collides with `UActorComponent`/`UObject` members or with any existing local in scope |
| most-vexing-parse | ✅ no declaration is parseable as a function; both new locals are copy-initialised from a named static (`FVector::ZeroVector`, `FVector2D::ZeroVector`) |
| complete-type includes | ✅ `SiegeAssistantRegionStatics.h` added at `:17` (alphabetical, in the `Siegebound/` block, with the reason on the line). `FVector`/`FVector2D` arrive complete through `CoreMinimal`; `ASummonedUnit` and `USiegeAssistantSnapshot` were already complete in this TU |
| broadcasting a delegate on a no-op | ✅ **no delegate is touched by this task at all** — every new path is a `UE_LOG` + `return false` |
| per-tick logging at default verbosity | ✅ `SelectUnitsForOrder` runs once per executed order (from `ExecuteZoneOrder`/`ExecuteFollowOrder`), never from a tick, timer or per-frame delegate |
| a new `TActorIterator` | ✅ **none.** The predicate is inside the one that already exists |
| a new `ask` symbol / reason code / template row | ✅ **none.** Both refusals route through the shipped `AskUnsupported` outcome via `return false` |

---

## 8. NOTES / DECLARED DEPARTURES

- 📌 **BOARD STATUS WRITTEN TWICE, AS SPECIFIED** — `backlog` → `in-progress` → `ready-for-qa`. The six Wave-1 siblings are all finished and TASK-549/550/551 are blocked on me, so I was the only writer of `TASKBOARD.md` in this window and the race TASK-544 declared did not apply.
- ⛔ **NO ACCURACY FIGURE IS ATTACHED TO ANYTHING HERE** — no predicted score, no row list, no percentage (`AS-§12f`). **Nobody has yet seen what the model emits for either of Jonathan's sentences**; TASK-542's raw-output log is what will show it, and §2 above is the reason that log now has a grammar worth reading.
- ⛔ **NO ZONE-A / PROMPT / CSV BYTE WAS TOUCHED.** This task adds **zero prompt characters**.
- 📌 **The `ValidateCommandAgainstSnapshot` observation, for the record and NOT acted on:** that guard (TASK-547's file, not mine) does not know about `RegionPlace`. It is **not a gap in practice** — the grammar can only emit a region symbol `GetRegionPlaceNames()` published this match, and my execution-time `ResolvePlaceRegion` refusal is the fail-closed backstop for the rest. Recorded so QA can rule rather than discover.
